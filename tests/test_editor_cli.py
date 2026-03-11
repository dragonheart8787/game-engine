import json
import subprocess
import sys
from pathlib import Path


CLI = [sys.executable, "-m", "game_engine.cli"]


def test_scene_cli_create_and_edit(tmp_path: Path) -> None:
    scene_file = tmp_path / "scene.json"

    subprocess.run(
        [*CLI, "scene-new", "--scene-guid", "demo", "--out", str(scene_file)],
        check=True,
        capture_output=True,
        text=True,
    )
    subprocess.run(
        [*CLI, "scene-add-entity", "--path", str(scene_file), "--entity-id", "player"],
        check=True,
        capture_output=True,
        text=True,
    )

    payload = json.loads(scene_file.read_text(encoding="utf-8"))
    assert payload["root_entities"] == ["player"]


def test_cli_reimport_decision() -> None:
    proc = subprocess.run(
        [
            *CLI,
            "asset-check-reimport",
            "--prev-source-hash",
            "a",
            "--prev-settings-hash",
            "b",
            "--source-path",
            "file.glb",
            "--source-hash",
            "a",
            "--settings-hash",
            "c",
        ],
        check=True,
        capture_output=True,
        text=True,
    )
    assert proc.stdout.strip() == "true"
