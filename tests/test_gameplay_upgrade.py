import json
import subprocess
import sys
from pathlib import Path

from game_engine.project.workflow import create_project, run_project
from game_engine.runtime.contracts import EngineContext
from game_engine.runtime.engine import RuntimeEngine

CLI = [sys.executable, "-m", "game_engine.cli"]


def test_runtime_engine_win_condition(tmp_path: Path) -> None:
    project_dir = tmp_path / "playable"
    create_project(str(project_dir), "playable")
    scene_path = project_dir / "scenes" / "main.scene.json"

    engine = RuntimeEngine()
    engine.initialize(EngineContext(startup_scene_id=str(scene_path), build_id="dev"))

    # Goal is at x=6, speed=3 units/sec, dt=1 sec -> reach in two moves
    frame1 = engine.tick(1000, input_action="right")
    frame2 = engine.tick(1000, input_action="right")

    assert frame1["won"] is False
    assert frame2["collected_coin"] is True
    assert frame2["score"] == 1
    assert frame2["won"] is True
    assert frame2["distance_to_goal"] <= 0.75


def test_run_project_with_input_script(tmp_path: Path) -> None:
    project_dir = tmp_path / "script_game"
    create_project(str(project_dir), "script_game")

    result = run_project(str(project_dir), frames=3, frame_time_ms=1000, input_script=["right", "right", "idle"])
    assert result["collected_coin"] is True
    assert result["score"] == 1
    assert result["won"] is True


def test_cli_run_game_with_input_script(tmp_path: Path) -> None:
    project_dir = tmp_path / "cli_game"
    create_project(str(project_dir), "cli_game")

    script = tmp_path / "input.json"
    script.write_text(json.dumps(["right", "right", "idle"]), encoding="utf-8")

    proc = subprocess.run(
        [
            *CLI,
            "run-game",
            "--project-dir",
            str(project_dir),
            "--frames",
            "3",
            "--frame-time",
            "1000",
            "--input-script",
            str(script),
        ],
        check=True,
        capture_output=True,
        text=True,
    )
    payload = json.loads(proc.stdout)
    assert payload["collected_coin"] is True
    assert payload["score"] == 1
    assert payload["won"] is True
    assert payload["player_x"] >= 6.0
