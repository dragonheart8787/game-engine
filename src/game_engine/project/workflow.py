"""End-to-end game project workflow helpers.

This module provides a practical local workflow so developers can scaffold,
run, and build a playable project from this repository.
"""

from __future__ import annotations

import json
from dataclasses import dataclass
from pathlib import Path

from game_engine.pipeline.bundler import write_bundle
from game_engine.pipeline.cook import cook_asset, transcode_texture
from game_engine.pipeline.importer import import_asset
from game_engine.pipeline.manifest_builder import manifest_hash
from game_engine.pipeline.schema import BuildManifest, BundleMetadata
from game_engine.runtime.contracts import EngineContext
from game_engine.runtime.engine import RuntimeEngine


@dataclass(slots=True)
class GameProject:
    name: str
    version: str
    startup_scene: str


def create_project(project_dir: str, name: str, version: str = "0.1.0") -> str:
    root = Path(project_dir)
    root.mkdir(parents=True, exist_ok=True)

    scenes = root / "scenes"
    assets = root / "assets"
    build = root / "build"
    for d in (scenes, assets, build):
        d.mkdir(parents=True, exist_ok=True)

    startup_scene = scenes / "main.scene.json"
    startup_scene.write_text(
        json.dumps(
            {
                "scene_id": "main",
                "entities": [
                    {"id": "player", "transform": {"x": 0, "y": 1, "z": 0}},
                    {"id": "coin", "transform": {"x": 3, "y": 1, "z": 0}},
                    {"id": "goal", "transform": {"x": 6, "y": 1, "z": 0}},
                    {"id": "camera", "transform": {"x": 0, "y": 2, "z": -6}},
                ],
            },
            indent=2,
            sort_keys=True,
        ),
        encoding="utf-8",
    )

    project = {
        "name": name,
        "version": version,
        "startup_scene": "scenes/main.scene.json",
    }
    (root / "game.project.json").write_text(json.dumps(project, indent=2, sort_keys=True), encoding="utf-8")
    (assets / "README.txt").write_text("Put source assets here.\n", encoding="utf-8")
    return str(root)


def load_project(project_dir: str) -> GameProject:
    payload = json.loads((Path(project_dir) / "game.project.json").read_text(encoding="utf-8"))
    return GameProject(name=payload["name"], version=payload["version"], startup_scene=payload["startup_scene"])


def run_project(project_dir: str, frames: int, frame_time_ms: float, input_script: list[str] | None = None) -> dict[str, float | int | bool]:
    project = load_project(project_dir)
    scene_path = Path(project_dir) / project.startup_scene
    engine = RuntimeEngine()
    engine.initialize(EngineContext(startup_scene_id=str(scene_path), build_id=f"{project.name}-{project.version}"))

    script = input_script or []
    result: dict[str, float | int | bool] = {}
    for frame_idx in range(frames):
        action = script[frame_idx] if frame_idx < len(script) else "idle"
        result = engine.tick(frame_time_ms, input_action=action)
    return result


def build_project(project_dir: str, output_dir: str, platform: str) -> dict[str, str]:
    root = Path(project_dir)
    out = Path(output_dir)
    out.mkdir(parents=True, exist_ok=True)

    project = load_project(project_dir)
    scene_source = root / project.startup_scene

    imported = import_asset(str(scene_source), {"profile": platform})
    cooked_path = cook_asset(str(scene_source), str(out / "startup_scene.cooked.gz"), platform)
    codec = transcode_texture(platform)

    manifest = BuildManifest(
        commit_sha="local",
        toolchain="python",
        platform=platform,
        input_hashes=[str(imported["source_hash"]), str(imported["settings_hash"])],
    )
    manifest_digest = manifest_hash(manifest)

    bundle_meta = BundleMetadata(
        bundle_id=project.name,
        semver=project.version,
        platform=platform,
        compression_mode="gzip",
        encryption_profile="none",
        content_manifest_hash=manifest_digest,
    )
    bundle_path = write_bundle(str(out), bundle_meta, [str(cooked_path)])

    return {
        "project": project.name,
        "platform": platform,
        "texture_codec": codec,
        "manifest_hash": manifest_digest,
        "bundle": bundle_path,
        "cooked_scene": cooked_path,
    }
