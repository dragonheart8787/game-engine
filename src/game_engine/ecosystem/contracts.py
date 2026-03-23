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


def is_compatible(engine_version: str, plugin_engine_version: str) -> bool:

    try:
        engine_major = int(engine_version.split(".")[0])
        plugin_major = int(plugin_engine_version.split(".")[0])
    except (ValueError, IndexError):
        return False
    return engine_major == plugin_major

