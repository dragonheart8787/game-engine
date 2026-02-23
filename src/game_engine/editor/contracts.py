"""Editor and authoring contracts."""

from dataclasses import dataclass


@dataclass(slots=True)
class SceneAsset:
    scene_guid: str
    version: int
    root_entities: list[str]


@dataclass(slots=True)
class ImportRequest:
    source_path: str
    source_hash: str
    settings_hash: str


@dataclass(slots=True)
class ImportResult:
    asset_guid: str
    artifact_refs: list[str]
    dependency_edges: list[tuple[str, str]]


def needs_reimport(previous_source_hash: str, previous_settings_hash: str, request: ImportRequest) -> bool:
    return (
        request.source_hash != previous_source_hash
        or request.settings_hash != previous_settings_hash
    )
