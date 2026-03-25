"""Plugin and package ecosystem contracts."""

from __future__ import annotations

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
    """Parse a semantic-ish version's major component safely."""
    if not version:
        return None

    major_text = version.strip().split(".", 1)[0]
    if not major_text.isdigit():
        return None

    return int(major_text)


def is_compatible(engine_version: str, plugin_engine_version: str) -> bool:
    """Return whether a plugin major version matches the engine major version."""
    engine_major = _parse_major(engine_version)
    plugin_major = _parse_major(plugin_engine_version)

    if engine_major is None or plugin_major is None:
        return False

    return engine_major == plugin_major
