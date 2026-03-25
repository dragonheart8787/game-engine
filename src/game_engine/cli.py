"""CLI entry points for validating repository contracts."""

from __future__ import annotations

import argparse
import json
from dataclasses import asdict, dataclass

from game_engine.architecture.platform import MVP_MILESTONES, PLATFORM_PILLARS


@dataclass(slots=True)
class CliReport:
    pillars: list[str]
    milestones: list[str]


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
    return parser


def render_report(report: CliReport, output_format: str) -> str:
    if output_format == "json":
        return json.dumps(asdict(report), indent=2, sort_keys=True)

    return (
        f"Pillars: {','.join(report.pillars)}\n"
        f"Milestones: {','.join(report.milestones)}"
    )


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    report = CliReport(
        pillars=list(PLATFORM_PILLARS),
        milestones=list(MVP_MILESTONES),
    )
    print(render_report(report, args.format))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
