"""CLI entry points for validating repository contracts and WeaveBound tool stubs."""

from __future__ import annotations

import argparse
import json
import sys
from dataclasses import asdict, dataclass
from pathlib import Path

from game_engine.architecture.platform import MVP_MILESTONES, PLATFORM_PILLARS
from game_engine.pipeline.rebuild import compare_manifests
from game_engine.pipeline.schema import BuildManifest


@dataclass(slots=True)
class CliReport:
    pillars: list[str]
    milestones: list[str]
    rebuild_report: dict[str, object] | None = None


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="game-engine",
        description="Inspect game engine platform contract metadata.",
    )
    parser.add_argument(
        "--format",
        choices=("text", "json"),
        default="text",
        help="Output format for platform metadata.",
    )
    parser.add_argument(
        "--old-input-hashes",
        default="",
        help="Comma-separated input hashes from previous build for rebuild comparison.",
    )
    parser.add_argument(
        "--new-input-hashes",
        default="",
        help="Comma-separated input hashes from current build for rebuild comparison.",
    )
    parser.add_argument("--old-commit", default="old")
    parser.add_argument("--new-commit", default="new")
    return parser


def render_report(report: CliReport, output_format: str) -> str:
    if output_format == "json":
        return json.dumps(asdict(report), indent=2, sort_keys=True)

    lines = [
        f"Pillars: {','.join(report.pillars)}",
        f"Milestones: {','.join(report.milestones)}",
    ]
    if report.rebuild_report is not None:
        lines.append(f"Rebuild added hashes: {','.join(report.rebuild_report['added_hashes'])}")
        lines.append(f"Rebuild removed hashes: {','.join(report.rebuild_report['removed_hashes'])}")
    return "\n".join(lines)


def _scene_tool_main(rest: list[str]) -> int:
    p = argparse.ArgumentParser(prog="python -m game_engine.cli scene_tool")
    p.add_argument(
        "action",
        choices=("validate", "bake-prefab", "bake-scene", "upgrade-scene"),
        help="validate: YAML scene check; bake-scene: 寫出 .wbscene 二進位（簡化）；其餘為 stub。",
    )
    p.add_argument(
        "--path",
        default=None,
        help="Scene YAML (validate / bake-scene) or asset path (stubs).",
    )
    p.add_argument(
        "--out",
        default=None,
        help="Output .wbscene path (bake-scene only).",
    )
    ns = p.parse_args(rest)
    if ns.action == "bake-prefab":
        print(
            "WeaveBound scene_tool bake-prefab stub: prefab binary bake not implemented "
            f"(path={ns.path!r})."
        )
        return 0
    if ns.action == "upgrade-scene":
        print(
            "WeaveBound scene_tool upgrade-scene stub: schema migration not implemented "
            f"(path={ns.path!r})."
        )
        return 0
    if ns.action == "bake-scene":
        if ns.path is None or ns.out is None:
            print("error: bake-scene requires --path and --out", file=sys.stderr)
            return 2
        import struct

        import yaml

        path = Path(ns.path)
        out = Path(ns.out)
        if not path.is_file():
            print(f"error: not a file: {path}", file=sys.stderr)
            return 1
        with path.open(encoding="utf-8") as f:
            data = yaml.safe_load(f)
        if not isinstance(data, dict):
            print("error: scene root must be a mapping", file=sys.stderr)
            return 1
        entities = data.get("entities")
        if not isinstance(entities, list):
            entities = []
        magic = 0x57425343
        version = 1
        body = bytearray()
        rec_sz = 4 + 4 + 4 * 3
        for ent in entities:
            if not isinstance(ent, dict):
                continue
            idx = int(ent.get("index", 0)) & 0xFFFFFFFF
            gen = int(ent.get("generation", 0)) & 0xFFFFFFFF
            pos = ent.get("position", [0.0, 0.0, 0.0])
            if not isinstance(pos, (list, tuple)) or len(pos) < 3:
                pos = [0.0, 0.0, 0.0]
            body.extend(struct.pack("<IIfff", idx, gen, float(pos[0]), float(pos[1]), float(pos[2])))
        count = len(body) // rec_sz
        blob = bytearray(struct.pack("<IIII", magic, version, count, 0))
        blob.extend(body)
        out.parent.mkdir(parents=True, exist_ok=True)
        out.write_bytes(blob)
        print(f"bake-scene: wrote {out} entities={count} bytes={len(blob)}")
        return 0
    if ns.path is None:
        print("error: validate requires --path", file=sys.stderr)
        return 2
    path = Path(ns.path)
    if not path.is_file():
        print(f"error: not a file: {path}", file=sys.stderr)
        return 1
    import yaml

    from game_engine.runtime.scene_validation import validate_scene

    with path.open(encoding="utf-8") as f:
        data = yaml.safe_load(f)
    if not isinstance(data, dict):
        print("error: scene root must be a mapping", file=sys.stderr)
        return 1
    report = validate_scene(data)
    for issue in report.issues:
        print(f"{issue.severity.value}: [{issue.code}] {issue.message} @ {issue.path}")
    return 0 if report.valid else 1


def _assetc_stub_main(rest: list[str]) -> int:
    import shutil
    import subprocess

    p = argparse.ArgumentParser(prog="python -m game_engine.cli assetc")
    p.add_argument("action", choices=("import", "cook", "build"))
    p.add_argument("--root", default=".", help="Project root (placeholder).")
    ns = p.parse_args(rest)
    if ns.action == "cook":
        cook_exe = shutil.which("weavebound_cook")
        if not cook_exe:
            b = Path(ns.root) / "build"
            if b.is_dir():
                for cand in b.rglob("weavebound_cook*.exe"):
                    if cand.is_file():
                        cook_exe = str(cand)
                        break
        out = Path(ns.root) / "cook_out" / "stub.wbmesh"
        out.parent.mkdir(parents=True, exist_ok=True)
        if cook_exe:
            proc = subprocess.run([cook_exe, str(out)], check=False)
            return int(proc.returncode)
        import struct

        magic = 0x57424D48
        header = struct.pack(
            "<IIIIII",
            magic,
            1,
            3,
            3,
            24,
            0,
        )
        out.write_bytes(header)
        print("assetc cook: wrote Python fallback stub (weavebound_cook not built).")
        return 0
    print(
        f"WeaveBound assetc stub: action={ns.action} root={ns.root!r}. "
        "See docs/WEAVEBOUND_SPEC.md sections 1.6 and 8."
    )
    return 0


def _packager_stub_main(rest: list[str]) -> int:
    p = argparse.ArgumentParser(prog="python -m game_engine.cli packager")
    p.add_argument("--platform", default="win64", help="Target platform label (placeholder).")
    p.add_argument("--out", default="dist", help="Output directory (placeholder).")
    p.add_argument(
        "--bundle-format",
        default="zip",
        choices=("zip", "apk", "ipa", "appimage"),
        help="Ship format (placeholder; spec 2.1 packager).",
    )
    ns = p.parse_args(rest)
    print(
        f"WeaveBound packager stub: platform={ns.platform!r} format={ns.bundle_format!r} "
        f"out={ns.out!r}. Bundle / hash manifest not implemented."
    )
    return 0


def main(argv: list[str] | None = None) -> int:
    raw = list(sys.argv[1:] if argv is None else argv)
    if raw and raw[0] == "scene_tool":
        return _scene_tool_main(raw[1:])
    if raw and raw[0] == "assetc":
        return _assetc_stub_main(raw[1:])
    if raw and raw[0] == "packager":
        return _packager_stub_main(raw[1:])

    args = build_parser().parse_args(raw)
    rebuild_report: dict[str, object] | None = None

    old_hashes = [h for h in args.old_input_hashes.split(",") if h]
    new_hashes = [h for h in args.new_input_hashes.split(",") if h]
    if old_hashes or new_hashes:
        old = BuildManifest(
            commit_sha=args.old_commit,
            toolchain="default",
            platform="cross",
            input_hashes=old_hashes,
        )
        new = BuildManifest(
            commit_sha=args.new_commit,
            toolchain="default",
            platform="cross",
            input_hashes=new_hashes,
        )
        rebuild_report = asdict(compare_manifests(old, new))

    report = CliReport(
        pillars=list(PLATFORM_PILLARS),
        milestones=list(MVP_MILESTONES),
        rebuild_report=rebuild_report,
    )
    print(render_report(report, args.format))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
