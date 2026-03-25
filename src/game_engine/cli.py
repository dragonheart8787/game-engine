"""CLI entry points for validating repository contracts."""

from __future__ import annotations

import argparse
import json
from dataclasses import asdict, dataclass

from game_engine.architecture.platform import MVP_MILESTONES, PLATFORM_PILLARS
from game_engine.ops.observability import MetricStore, dump_metrics
from game_engine.pipeline.rebuild import compare_manifests
from game_engine.pipeline.schema import BuildManifest


@dataclass(slots=True)
class CliReport:
    pillars: list[str]
    milestones: list[str]
    rebuild_report: dict[str, object] | None = None
    metrics_dump: str | None = None


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
    parser.add_argument(
        "--dump-metrics",
        action="store_true",
        help="Emit a JSON metrics snapshot for observability wiring checks.",
    )
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
    if report.metrics_dump is not None:
        lines.append(f"Metrics: {report.metrics_dump}")
    return "\n".join(lines)


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
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

    metrics_dump: str | None = None
    if args.dump_metrics:
        metrics = MetricStore()
        metrics.inc("cli.invocations")
        metrics.set_gauge("cli.rebuild_hash_count", float(len(old_hashes) + len(new_hashes)))
        metrics.emit("cli.report_generated", format=args.format)
        metrics_dump = dump_metrics(metrics)

    report = CliReport(
        pillars=list(PLATFORM_PILLARS),
        milestones=list(MVP_MILESTONES),
        rebuild_report=rebuild_report,
        metrics_dump=metrics_dump,
    )
    print(render_report(report, args.format))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
