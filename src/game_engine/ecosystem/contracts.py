"""Plugin and package ecosystem contracts."""

from dataclasses import dataclass


@dataclass(slots=True)
class PluginManifest:
    name: str
    version: str
    engine_version: str
    entrypoint: str
    capabilities: list[str]
    dependencies: list[str]
    permissions: list[str]


def _parse_major(version: str) -> int | None:
    """Extract a numeric major version from semver-like strings.

    Accepts optional leading ``v`` and ignores pre-release/build metadata.
    Returns ``None`` when the major component is missing or non-numeric.
    """

    cleaned = version.strip()
    if cleaned.startswith(("v", "V")):
        cleaned = cleaned[1:]

    major = cleaned.split(".", 1)[0]
    if not major.isdigit():
        return None
    return int(major)


def is_compatible(engine_version: str, plugin_engine_version: str) -> bool:
    engine_major = _parse_major(engine_version)
    plugin_major = _parse_major(plugin_engine_version)
    if engine_major is None or plugin_major is None:
        return False
    return engine_major == plugin_major
