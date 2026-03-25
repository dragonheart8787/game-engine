"""Validators for editor artifacts and import requests."""

from __future__ import annotations

from dataclasses import dataclass

from .contracts import ImportRequest, SceneAsset


@dataclass(frozen=True, slots=True)
class ValidationIssue:
    code: str
    message: str


def validate_scene_asset(scene: SceneAsset) -> list[ValidationIssue]:
    issues: list[ValidationIssue] = []
    if scene.version <= 0:
        issues.append(ValidationIssue("scene.version.invalid", "Scene version must be positive"))
    if not scene.scene_guid.strip():
        issues.append(ValidationIssue("scene.guid.empty", "Scene GUID must be non-empty"))
    if len(scene.root_entities) != len(set(scene.root_entities)):
        issues.append(ValidationIssue("scene.entities.duplicate", "Root entities must be unique"))
    return issues


def validate_import_request(request: ImportRequest) -> list[ValidationIssue]:
    issues: list[ValidationIssue] = []
    if not request.source_path:
        issues.append(ValidationIssue("import.path.empty", "Source path is required"))
    if len(request.source_hash) < 2:
        issues.append(ValidationIssue("import.source_hash.short", "Source hash appears invalid"))
    if len(request.settings_hash) < 2:
        issues.append(ValidationIssue("import.settings_hash.short", "Settings hash appears invalid"))
    return issues
