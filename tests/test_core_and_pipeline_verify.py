import json
import subprocess
import sys
from pathlib import Path

from game_engine.core.config import apply_override
from game_engine.core.events import EventBus
from game_engine.core.jobs import JobSystem
from game_engine.pipeline.verify import verify_patch_compatibility, verify_reproducible_manifest

CLI = [sys.executable, "-m", "game_engine.cli"]


def test_core_modules() -> None:
    merged = apply_override({"a": 1}, {"b": 2, "a": 3})
    assert merged["a"] == 3
    assert merged["b"] == 2

    bus = EventBus()
    seen: list[int] = []
    bus.subscribe("tick", lambda p: seen.append(int(p["n"])))
    bus.emit("tick", {"n": 1})
    assert seen == [1]

    jobs = JobSystem(workers=2)
    try:
        assert jobs.run(lambda: 7 * 6) == 42
    finally:
        jobs.shutdown()


def test_pipeline_verify_helpers(tmp_path: Path) -> None:
    a = tmp_path / "a.json"
    b = tmp_path / "b.json"
    a.write_text(json.dumps({"x": 1}, sort_keys=True), encoding="utf-8")
    b.write_text(json.dumps({"x": 1}, sort_keys=True), encoding="utf-8")
    assert verify_reproducible_manifest(str(a), str(b))
    assert verify_patch_compatibility("1.0.0", "1.2.0")
    assert not verify_patch_compatibility("1.2.0", "2.0.0")


def test_new_cli_commands(tmp_path: Path) -> None:
    a = tmp_path / "a.json"
    b = tmp_path / "b.json"
    cfg = tmp_path / "cfg.json"
    a.write_text(json.dumps({"m": 1}), encoding="utf-8")
    b.write_text(json.dumps({"m": 1}), encoding="utf-8")
    cfg.write_text(json.dumps({"difficulty": "normal"}), encoding="utf-8")

    out_verify = subprocess.run([*CLI, "pipeline-verify", "--manifest-a", str(a), "--manifest-b", str(b)], check=True, capture_output=True, text=True)
    assert out_verify.stdout.strip() == "true"

    out_patch = subprocess.run([*CLI, "patch-verify", "--base", "1.0.0", "--target", "1.1.0"], check=True, capture_output=True, text=True)
    assert out_patch.stdout.strip() == "true"

    out_cfg = subprocess.run([
        *CLI,
        "config-merge",
        "--base",
        str(cfg),
        "--override-json",
        json.dumps({"difficulty": "hard", "vsync": True}),
    ], check=True, capture_output=True, text=True)
    payload = json.loads(out_cfg.stdout)
    assert payload["difficulty"] == "hard"
    assert payload["vsync"] is True

    out_event = subprocess.run([*CLI, "event-smoke"], check=True, capture_output=True, text=True)
    assert out_event.stdout.strip() == "1,2"

    out_job = subprocess.run([*CLI, "job-smoke", "--value", "9"], check=True, capture_output=True, text=True)
    assert out_job.stdout.strip() == "81"
