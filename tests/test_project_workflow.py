import json
import subprocess
import sys
from pathlib import Path

from game_engine.project.workflow import build_project, create_project, run_project

CLI = [sys.executable, "-m", "game_engine.cli"]


def test_project_workflow_python_api(tmp_path: Path) -> None:
    project_dir = tmp_path / "space_game"
    create_project(str(project_dir), name="space_game", version="0.2.0")

    run_result = run_project(str(project_dir), frames=5, frame_time_ms=16.6)
    assert run_result["frame_index"] == 5
    assert run_result["draw_calls"] >= 1

    build_result = build_project(str(project_dir), str(project_dir / "dist"), platform="desktop")
    assert build_result["texture_codec"] == "BC7"
    assert Path(build_result["bundle"]).exists()
    assert Path(build_result["cooked_scene"]).exists()


def test_project_workflow_cli(tmp_path: Path) -> None:
    project_dir = tmp_path / "my_game"
    dist_dir = project_dir / "dist"

    subprocess.run(
        [*CLI, "new-game", "--project-dir", str(project_dir), "--name", "my_game", "--version", "0.3.0"],
        check=True,
        capture_output=True,
        text=True,
    )
    assert (project_dir / "game.project.json").exists()

    run_cmd = subprocess.run(
        [*CLI, "run-game", "--project-dir", str(project_dir), "--frames", "3", "--frame-time", "16.6"],
        check=True,
        capture_output=True,
        text=True,
    )
    run_payload = json.loads(run_cmd.stdout)
    assert run_payload["frame_index"] == 3

    build_cmd = subprocess.run(
        [*CLI, "build-game", "--project-dir", str(project_dir), "--out-dir", str(dist_dir), "--platform", "mobile"],
        check=True,
        capture_output=True,
        text=True,
    )
    build_payload = json.loads(build_cmd.stdout)
    assert build_payload["texture_codec"] == "ASTC"
    assert Path(build_payload["bundle"]).exists()
