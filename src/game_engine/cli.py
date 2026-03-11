"""CLI entry points for validating repository contracts."""

from __future__ import annotations

import argparse
import json
from pathlib import Path

from game_engine.architecture.platform import MVP_MILESTONES, PLATFORM_PILLARS
from game_engine.core.config import apply_override, load_config
from game_engine.core.events import EventBus
from game_engine.core.jobs import JobSystem
from game_engine.ecosystem.contracts import PluginManifest
from game_engine.ecosystem.validators import validate_plugin_manifest
from game_engine.editor.contracts import ImportRequest, needs_reimport
from game_engine.editor.scene_io import add_entity, create_scene, load_scene, save_scene
from game_engine.ops.release_ops import ingest_crash, make_dlc_manifest, make_patch_manifest
from game_engine.pipeline.cook import cook_asset, transcode_texture
from game_engine.pipeline.verify import verify_patch_compatibility, verify_reproducible_manifest
from game_engine.perf.benchmark import evaluate_thresholds, results_to_csv, run_case, save_results_json
from game_engine.project.workflow import build_project, create_project, run_project
from game_engine.runtime.contracts import EngineContext
from game_engine.runtime.engine import RuntimeEngine
from game_engine.runtime.render_backends import RenderBackend, create_backend
from game_engine.services.contracts import AuthTokenClaims
from game_engine.services.validators import validate_auth_claims


def _cmd_info() -> None:
    print("Pillars:", ",".join(PLATFORM_PILLARS))
    print("Milestones:", ",".join(MVP_MILESTONES))


def _cmd_run_demo(scene: str, frames: int, frame_time: float) -> None:
    engine = RuntimeEngine()
    engine.initialize(EngineContext(startup_scene_id=scene, build_id="dev"))
    result = {}
    for _ in range(frames):
        result = engine.tick(frame_time)
    print(json.dumps(result, sort_keys=True))


def _cmd_scene_new(scene_guid: str, out: str) -> None:
    scene = create_scene(scene_guid)
    save_scene(scene, out)
    print(out)


def _cmd_scene_add_entity(path: str, entity_id: str) -> None:
    scene = load_scene(path)
    scene = add_entity(scene, entity_id)
    save_scene(scene, path)
    print(path)


def _cmd_check_reimport(prev_source_hash: str, prev_settings_hash: str, source_path: str, source_hash: str, settings_hash: str) -> None:
    req = ImportRequest(source_path=source_path, source_hash=source_hash, settings_hash=settings_hash)
    print(str(needs_reimport(prev_source_hash, prev_settings_hash, req)).lower())


def _cmd_validate_plugin_manifest(path: str) -> None:
    data = json.loads(Path(path).read_text(encoding="utf-8"))
    manifest = PluginManifest(**data)
    errors = validate_plugin_manifest(manifest)
    print("ok" if not errors else "; ".join(errors))


def _cmd_validate_auth_token(path: str) -> None:
    data = json.loads(Path(path).read_text(encoding="utf-8"))
    claims = AuthTokenClaims(**data)
    errors = validate_auth_claims(claims)
    print("ok" if not errors else "; ".join(errors))


def _cmd_render_backend(backend: str, draw_calls: int, frame_index: int) -> None:
    selected = create_backend(RenderBackend(backend))
    frame = selected.encode_frame(frame_index=frame_index, draw_calls=draw_calls)
    print(json.dumps({"backend": frame.backend.value, "draw_calls": frame.draw_calls, "commands": frame.command_buffer}, sort_keys=True))


def _cmd_cook_asset(source: str, out: str, platform: str) -> None:
    cooked = cook_asset(source, out, platform)
    texture_codec = transcode_texture(platform)
    print(json.dumps({"cooked": cooked, "texture_codec": texture_codec}, sort_keys=True))


def _cmd_make_patch(base: str, target: str, out: str, files: list[str]) -> None:
    print(make_patch_manifest(base, target, files, out))


def _cmd_make_dlc(version: str, dlc_id: str, out: str, files: list[str]) -> None:
    print(make_dlc_manifest(version, dlc_id, files, out))


def _cmd_ingest_crash(payload_path: str, out_dir: str) -> None:
    payload = json.loads(Path(payload_path).read_text(encoding="utf-8"))
    print(ingest_crash(payload, out_dir))


def _cmd_new_game(project_dir: str, name: str, version: str) -> None:
    print(create_project(project_dir=project_dir, name=name, version=version))


def _load_input_script(path: str | None) -> list[str]:
    if not path:
        return []
    payload = json.loads(Path(path).read_text(encoding="utf-8"))
    if not isinstance(payload, list):
        raise ValueError("input script must be a JSON array of actions")
    return [str(item) for item in payload]


def _cmd_run_game(project_dir: str, frames: int, frame_time: float, input_script_path: str | None) -> None:
    script = _load_input_script(input_script_path)
    print(json.dumps(run_project(project_dir, frames=frames, frame_time_ms=frame_time, input_script=script), sort_keys=True))


def _cmd_build_game(project_dir: str, out_dir: str, platform: str) -> None:
    print(json.dumps(build_project(project_dir=project_dir, output_dir=out_dir, platform=platform), sort_keys=True))


def _cmd_pipeline_verify(manifest_a: str, manifest_b: str) -> None:
    print(str(verify_reproducible_manifest(manifest_a, manifest_b)).lower())


def _cmd_patch_verify(base: str, target: str) -> None:
    print(str(verify_patch_compatibility(base, target)).lower())


def _cmd_config_merge(base_path: str, override_json: str) -> None:
    base = load_config(base_path)
    override = json.loads(override_json)
    print(json.dumps(apply_override(base, override), sort_keys=True))


def _cmd_event_smoke() -> None:
    bus = EventBus()
    seen: list[str] = []
    bus.subscribe("frame", lambda payload: seen.append(str(payload["id"])))
    bus.emit("frame", {"id": 1})
    bus.emit("frame", {"id": 2})
    print(",".join(seen))


def _cmd_job_smoke(value: int) -> None:
    jobs = JobSystem(workers=2)
    try:
        result = jobs.run(lambda: value * value)
    finally:
        jobs.shutdown()
    print(result)


def _cmd_benchmark_run(project_dir: str, out_json: str, out_csv: str, win_script: str) -> None:
    results = [
        run_case(project_dir, case="idle", frames=300, frame_time_ms=16.6),
        run_case(project_dir, case="win_path", frames=3, frame_time_ms=1000, input_script_path=win_script),
        run_case(project_dir, case="scale_100", frames=120, frame_time_ms=16.6, extra_entities=100),
    ]
    json_path = save_results_json(results, out_json)
    csv_path = results_to_csv(json_path, out_csv)
    print(json.dumps({"results_json": json_path, "results_csv": csv_path}, sort_keys=True))


def _cmd_benchmark_gate(results: str, thresholds: str) -> None:
    ok, errors = evaluate_thresholds(results, thresholds)
    if ok:
        print("pass")
        return
    print("fail:" + " | ".join(errors))
    raise SystemExit(1)


def main() -> None:
    parser = argparse.ArgumentParser()
    sub = parser.add_subparsers(dest="command", required=True)

    sub.add_parser("info")

    run_demo = sub.add_parser("run-demo")
    run_demo.add_argument("--scene", required=True)
    run_demo.add_argument("--frames", type=int, default=3)
    run_demo.add_argument("--frame-time", type=float, default=16.6)

    scene_new = sub.add_parser("scene-new")
    scene_new.add_argument("--scene-guid", required=True)
    scene_new.add_argument("--out", required=True)

    scene_add = sub.add_parser("scene-add-entity")
    scene_add.add_argument("--path", required=True)
    scene_add.add_argument("--entity-id", required=True)

    check_reimport = sub.add_parser("asset-check-reimport")
    check_reimport.add_argument("--prev-source-hash", required=True)
    check_reimport.add_argument("--prev-settings-hash", required=True)
    check_reimport.add_argument("--source-path", required=True)
    check_reimport.add_argument("--source-hash", required=True)
    check_reimport.add_argument("--settings-hash", required=True)

    val_plugin = sub.add_parser("validate-plugin-manifest")
    val_plugin.add_argument("--path", required=True)

    val_auth = sub.add_parser("validate-auth-token")
    val_auth.add_argument("--path", required=True)

    render_backend = sub.add_parser("render-backend")
    render_backend.add_argument("--backend", choices=["vulkan", "dx12", "metal"], required=True)
    render_backend.add_argument("--draw-calls", type=int, default=1)
    render_backend.add_argument("--frame-index", type=int, default=1)

    cook = sub.add_parser("cook-asset")
    cook.add_argument("--source", required=True)
    cook.add_argument("--out", required=True)
    cook.add_argument("--platform", required=True)

    patch = sub.add_parser("make-patch")
    patch.add_argument("--base", required=True)
    patch.add_argument("--target", required=True)
    patch.add_argument("--out", required=True)
    patch.add_argument("files", nargs="+")

    dlc = sub.add_parser("make-dlc")
    dlc.add_argument("--version", required=True)
    dlc.add_argument("--dlc-id", required=True)
    dlc.add_argument("--out", required=True)
    dlc.add_argument("files", nargs="+")

    ingest = sub.add_parser("ingest-crash")
    ingest.add_argument("--payload", required=True)
    ingest.add_argument("--out-dir", required=True)

    game_new = sub.add_parser("new-game")
    game_new.add_argument("--project-dir", required=True)
    game_new.add_argument("--name", required=True)
    game_new.add_argument("--version", default="0.1.0")

    game_run = sub.add_parser("run-game")
    game_run.add_argument("--project-dir", required=True)
    game_run.add_argument("--frames", type=int, default=120)
    game_run.add_argument("--frame-time", type=float, default=16.6)
    game_run.add_argument("--input-script", default=None)

    game_build = sub.add_parser("build-game")
    game_build.add_argument("--project-dir", required=True)
    game_build.add_argument("--out-dir", required=True)
    game_build.add_argument("--platform", required=True)

    pipe_verify = sub.add_parser("pipeline-verify")
    pipe_verify.add_argument("--manifest-a", required=True)
    pipe_verify.add_argument("--manifest-b", required=True)

    patch_verify = sub.add_parser("patch-verify")
    patch_verify.add_argument("--base", required=True)
    patch_verify.add_argument("--target", required=True)

    cfg_merge = sub.add_parser("config-merge")
    cfg_merge.add_argument("--base", required=True)
    cfg_merge.add_argument("--override-json", required=True)

    event_smoke = sub.add_parser("event-smoke")

    job_smoke = sub.add_parser("job-smoke")
    job_smoke.add_argument("--value", type=int, default=4)

    bench_run = sub.add_parser("benchmark-run")
    bench_run.add_argument("--project-dir", required=True)
    bench_run.add_argument("--out-json", required=True)
    bench_run.add_argument("--out-csv", required=True)
    bench_run.add_argument("--win-script", required=True)

    bench_gate = sub.add_parser("benchmark-gate")
    bench_gate.add_argument("--results", required=True)
    bench_gate.add_argument("--thresholds", required=True)

    args = parser.parse_args()

    if args.command == "info":
        _cmd_info()
    elif args.command == "run-demo":
        _cmd_run_demo(args.scene, args.frames, args.frame_time)
    elif args.command == "scene-new":
        _cmd_scene_new(args.scene_guid, args.out)
    elif args.command == "scene-add-entity":
        _cmd_scene_add_entity(args.path, args.entity_id)
    elif args.command == "asset-check-reimport":
        _cmd_check_reimport(args.prev_source_hash, args.prev_settings_hash, args.source_path, args.source_hash, args.settings_hash)
    elif args.command == "validate-plugin-manifest":
        _cmd_validate_plugin_manifest(args.path)
    elif args.command == "validate-auth-token":
        _cmd_validate_auth_token(args.path)
    elif args.command == "render-backend":
        _cmd_render_backend(args.backend, args.draw_calls, args.frame_index)
    elif args.command == "cook-asset":
        _cmd_cook_asset(args.source, args.out, args.platform)
    elif args.command == "make-patch":
        _cmd_make_patch(args.base, args.target, args.out, args.files)
    elif args.command == "make-dlc":
        _cmd_make_dlc(args.version, args.dlc_id, args.out, args.files)
    elif args.command == "ingest-crash":
        _cmd_ingest_crash(args.payload, args.out_dir)
    elif args.command == "new-game":
        _cmd_new_game(args.project_dir, args.name, args.version)
    elif args.command == "run-game":
        _cmd_run_game(args.project_dir, args.frames, args.frame_time, args.input_script)
    elif args.command == "build-game":
        _cmd_build_game(args.project_dir, args.out_dir, args.platform)
    elif args.command == "pipeline-verify":
        _cmd_pipeline_verify(args.manifest_a, args.manifest_b)
    elif args.command == "patch-verify":
        _cmd_patch_verify(args.base, args.target)
    elif args.command == "config-merge":
        _cmd_config_merge(args.base, args.override_json)
    elif args.command == "event-smoke":
        _cmd_event_smoke()
    elif args.command == "job-smoke":
        _cmd_job_smoke(args.value)
    elif args.command == "benchmark-run":
        _cmd_benchmark_run(args.project_dir, args.out_json, args.out_csv, args.win_script)
    elif args.command == "benchmark-gate":
        _cmd_benchmark_gate(args.results, args.thresholds)


if __name__ == "__main__":
    main()
