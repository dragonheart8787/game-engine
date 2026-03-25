"""Patch manifest and rollback map handling."""

from __future__ import annotations

from dataclasses import dataclass


@dataclass(slots=True)
class PatchManifest:
    base_manifest_hash: str
    patch_manifest_hash: str
    changed_assets: dict[str, str]


def apply_rollback_map(
    manifest: PatchManifest,
    rollback_map: dict[str, str],
) -> PatchManifest:
    """Apply rollback mapping (asset -> previous hash) when asset exists in manifest."""
    updated = dict(manifest.changed_assets)
    for asset_guid, rollback_hash in rollback_map.items():
        if asset_guid in updated:
            updated[asset_guid] = rollback_hash
    return PatchManifest(
        base_manifest_hash=manifest.base_manifest_hash,
        patch_manifest_hash=manifest.patch_manifest_hash,
        changed_assets=updated,
    )


def validate_rollback_map(manifest: PatchManifest, rollback_map: dict[str, str]) -> list[str]:
    issues: list[str] = []
    for asset_guid, rollback_hash in rollback_map.items():
        if asset_guid not in manifest.changed_assets:
            issues.append(f"unknown asset '{asset_guid}'")
        if not rollback_hash:
            issues.append(f"asset '{asset_guid}' rollback hash missing")
    return issues
