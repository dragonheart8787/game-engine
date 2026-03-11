import json
import subprocess
import sys
from pathlib import Path

from game_engine.perf.benchmark import evaluate_thresholds, results_to_csv, run_case, save_results_json
from game_engine.project.workflow import create_project

CLI = [sys.executable, "-m", "game_engine.cli"]


def test_benchmark_module_flow(tmp_path: Path) -> None:
    project = tmp_path / "bench_game"
    create_project(str(project), name="bench_game")

    idle = run_case(str(project), case="idle", frames=10, frame_time_ms=16.6)
    win = run_case(
        str(project),
        case="win_path",
        frames=3,
        frame_time_ms=1000,
        input_script_path="examples/minimal-game/input_win_script.json",
    )

    out_json = tmp_path / "results.json"
    out_csv = tmp_path / "results.csv"
    save_results_json([idle, win], str(out_json))
    results_to_csv(str(out_json), str(out_csv))
    assert out_json.exists()
    assert out_csv.exists()


def test_benchmark_gate(tmp_path: Path) -> None:
    results = tmp_path / "results.json"
    results.write_text(
        json.dumps(
            {
                "cases": [
                    {"case": "idle", "metrics": {"phase_input_ms": 0.01, "phase_gameplay_ms": 0.01, "phase_render_ms": 0.01}},
                ]
            }
        ),
        encoding="utf-8",
    )
    thresholds = tmp_path / "thresholds.json"
    thresholds.write_text(
        json.dumps({"cases": {"idle": {"phase_input_ms": 0.1, "phase_gameplay_ms": 0.1, "phase_render_ms": 0.1}}}),
        encoding="utf-8",
    )

    ok, errors = evaluate_thresholds(str(results), str(thresholds))
    assert ok is True
    assert errors == []


def test_benchmark_cli_commands(tmp_path: Path) -> None:
    project = tmp_path / "bench_cli"
    out_json = tmp_path / "bench_results.json"
    out_csv = tmp_path / "bench_results.csv"

    subprocess.run(
        [*CLI, "new-game", "--project-dir", str(project), "--name", "bench_cli", "--version", "0.1.0"],
        check=True,
        capture_output=True,
        text=True,
    )
    run = subprocess.run(
        [
            *CLI,
            "benchmark-run",
            "--project-dir",
            str(project),
            "--out-json",
            str(out_json),
            "--out-csv",
            str(out_csv),
            "--win-script",
            "examples/minimal-game/input_win_script.json",
        ],
        check=True,
        capture_output=True,
        text=True,
    )
    payload = json.loads(run.stdout)
    assert Path(payload["results_json"]).exists()
    assert Path(payload["results_csv"]).exists()

    gate = subprocess.run(
        [*CLI, "benchmark-gate", "--results", str(out_json), "--thresholds", "benchmarks/thresholds.json"],
        check=True,
        capture_output=True,
        text=True,
    )
    assert gate.stdout.strip() == "pass"
