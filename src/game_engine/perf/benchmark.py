"""Benchmark helpers for runtime performance baselines and regression gates."""

from __future__ import annotations

import csv
import json
import shutil
import tempfile
from dataclasses import dataclass
from pathlib import Path

from game_engine.project.workflow import load_project, run_project


@dataclass(slots=True)
class BenchmarkResult:
    case: str
    frames: int
    frame_time_ms: float
    result: dict[str, float | int | bool]


def _load_script(path: str | None) -> list[str]:
    if not path:
        return []
    return [str(x) for x in json.loads(Path(path).read_text(encoding="utf-8"))]


def _scaled_project_copy(project_dir: str, extra_entities: int) -> str:
    if extra_entities <= 0:
        return project_dir

    temp_dir = tempfile.mkdtemp(prefix="ge-bench-")
    dst = Path(temp_dir) / "project"
    shutil.copytree(project_dir, dst)

    project = load_project(str(dst))
    scene_path = dst / project.startup_scene
    scene = json.loads(scene_path.read_text(encoding="utf-8"))
    entities = scene.setdefault("entities", [])
    for i in range(extra_entities):
        entities.append({"id": f"dummy_{i}", "transform": {"x": i % 100, "y": 0, "z": i // 100}})
    scene_path.write_text(json.dumps(scene, sort_keys=True, indent=2), encoding="utf-8")
    return str(dst)


def run_case(
    project_dir: str,
    case: str,
    frames: int,
    frame_time_ms: float,
    input_script_path: str | None = None,
    extra_entities: int = 0,
) -> BenchmarkResult:
    bench_project = _scaled_project_copy(project_dir, extra_entities)
    script = _load_script(input_script_path)
    result = run_project(bench_project, frames=frames, frame_time_ms=frame_time_ms, input_script=script)
    return BenchmarkResult(case=case, frames=frames, frame_time_ms=frame_time_ms, result=result)


def save_results_json(results: list[BenchmarkResult], output_path: str) -> str:
    payload = {
        "cases": [
            {
                "case": r.case,
                "frames": r.frames,
                "frame_time_ms": r.frame_time_ms,
                "metrics": r.result,
            }
            for r in results
        ]
    }
    out = Path(output_path)
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(payload, sort_keys=True, indent=2), encoding="utf-8")
    return str(out)


def results_to_csv(results_json: str, csv_path: str) -> str:
    payload = json.loads(Path(results_json).read_text(encoding="utf-8"))
    out = Path(csv_path)
    out.parent.mkdir(parents=True, exist_ok=True)

    fields = [
        "case",
        "frames",
        "frame_time_ms",
        "phase_input_ms",
        "phase_gameplay_ms",
        "phase_render_ms",
        "draw_calls",
        "rendered_entities",
        "resources",
        "resource_refs",
        "won",
        "score",
    ]
    with out.open("w", newline="", encoding="utf-8") as fh:
        writer = csv.DictWriter(fh, fieldnames=fields)
        writer.writeheader()
        for case in payload.get("cases", []):
            m = case.get("metrics", {})
            writer.writerow(
                {
                    "case": case.get("case"),
                    "frames": case.get("frames"),
                    "frame_time_ms": case.get("frame_time_ms"),
                    "phase_input_ms": m.get("phase_input_ms"),
                    "phase_gameplay_ms": m.get("phase_gameplay_ms"),
                    "phase_render_ms": m.get("phase_render_ms"),
                    "draw_calls": m.get("draw_calls"),
                    "rendered_entities": m.get("rendered_entities"),
                    "resources": m.get("resources"),
                    "resource_refs": m.get("resource_refs"),
                    "won": m.get("won"),
                    "score": m.get("score"),
                }
            )
    return str(out)


def evaluate_thresholds(results_json: str, thresholds_json: str) -> tuple[bool, list[str]]:
    results = json.loads(Path(results_json).read_text(encoding="utf-8"))
    thresholds = json.loads(Path(thresholds_json).read_text(encoding="utf-8"))

    errors: list[str] = []
    case_map = {case["case"]: case.get("metrics", {}) for case in results.get("cases", [])}
    for case_name, limits in thresholds.get("cases", {}).items():
        metrics = case_map.get(case_name)
        if metrics is None:
            errors.append(f"missing case '{case_name}'")
            continue
        for metric, max_value in limits.items():
            value = metrics.get(metric)
            if value is None:
                errors.append(f"{case_name}: missing metric '{metric}'")
                continue
            if float(value) > float(max_value):
                errors.append(f"{case_name}: {metric}={value} > {max_value}")
    return (len(errors) == 0, errors)
