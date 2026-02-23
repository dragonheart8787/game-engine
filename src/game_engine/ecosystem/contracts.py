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


def is_compatible(engine_version: str, plugin_engine_version: str) -> bool:
    return engine_version.split(".")[0] == plugin_engine_version.split(".")[0]
