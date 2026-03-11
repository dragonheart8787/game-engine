import json
import subprocess
import sys
from pathlib import Path

from game_engine.ops.release_ops import ingest_crash, make_dlc_manifest, make_patch_manifest
from game_engine.pipeline.cook import cook_asset, transcode_texture

CLI = [sys.executable, "-m", "game_engine.cli"]


def test_cook_and_release_ops(tmp_path: Path) -> None:
    source = tmp_path / "asset.txt"
    source.write_text("hello", encoding="utf-8")

    cooked = cook_asset(str(source), str(tmp_path / "cooked.bin.gz"), platform="desktop")
    assert Path(cooked).exists()
    assert transcode_texture("mobile") == "ASTC"

    patch = make_patch_manifest("1.0.0", "1.0.1", ["a.bin"], str(tmp_path / "patch.json"))
    dlc = make_dlc_manifest("1.0.1", "dlc-1", ["d.bin"], str(tmp_path / "dlc.json"))
    crash = ingest_crash({"build_id": "b1", "stack_fingerprint": "f1"}, str(tmp_path / "crash"))

    assert Path(patch).exists()
    assert Path(dlc).exists()
    assert Path(crash).exists()


def test_new_cli_commands(tmp_path: Path) -> None:
    source = tmp_path / "asset.txt"
    source.write_text("hello", encoding="utf-8")

    render = subprocess.run([*CLI, "render-backend", "--backend", "vulkan", "--draw-calls", "2"], check=True, capture_output=True, text=True)
    render_payload = json.loads(render.stdout)
    assert render_payload["backend"] == "vulkan"

    cooked = subprocess.run(
        [*CLI, "cook-asset", "--source", str(source), "--out", str(tmp_path / "out.gz"), "--platform", "mobile"],
        check=True,
        capture_output=True,
        text=True,
    )
    assert "ASTC" in cooked.stdout

    patch = subprocess.run(
        [*CLI, "make-patch", "--base", "1.0.0", "--target", "1.0.1", "--out", str(tmp_path / "p.json"), "a.bin"],
        check=True,
        capture_output=True,
        text=True,
    )
    assert patch.stdout.strip().endswith("p.json")

    crash_file = tmp_path / "crash.json"
    crash_file.write_text(Path("examples/ops/crash_payload.json").read_text(encoding="utf-8"), encoding="utf-8")
    ingest = subprocess.run(
        [*CLI, "ingest-crash", "--payload", str(crash_file), "--out-dir", str(tmp_path / "ingested")],
        check=True,
        capture_output=True,
        text=True,
    )
    assert Path(ingest.stdout.strip()).exists()
