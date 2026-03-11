"""Import orchestration wrappers for editor tooling."""

from __future__ import annotations

from .contracts import ImportRequest, ImportResult


def build_import_result(request: ImportRequest, asset_guid: str) -> ImportResult:
    return ImportResult(
        asset_guid=asset_guid,
        artifact_refs=[f"intermediate/{asset_guid}.bin"],
        dependency_edges=[(request.source_path, asset_guid)],
    )
