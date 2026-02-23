"""CLI entry points for validating repository contracts."""

from game_engine.architecture.platform import MVP_MILESTONES, PLATFORM_PILLARS


def main() -> None:
    print("Pillars:", ",".join(PLATFORM_PILLARS))
    print("Milestones:", ",".join(MVP_MILESTONES))


if __name__ == "__main__":
    main()
