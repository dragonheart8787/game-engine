"""Rebuild comparison and reporting for manifests/input hashes."""

from __future__ import annotations

from dataclasses import dataclass

from .schema import BuildManifest


@dataclass(slots=True)
class RebuildReport:
    old_commit: str
    new_commit: str
    added_hashes: list[str]
    removed_hashes: list[str]
    unchanged_hashes: list[str]
    toolchain_changed: bool
    platform_changed: bool


def compare_manifests(old: BuildManifest, new: BuildManifest) -> RebuildReport:
    old_set = set(old.input_hashes)
    new_set = set(new.input_hashes)
    return RebuildReport(
        old_commit=old.commit_sha,
        new_commit=new.commit_sha,
        added_hashes=sorted(new_set - old_set),
        removed_hashes=sorted(old_set - new_set),
        unchanged_hashes=sorted(old_set & new_set),
        toolchain_changed=old.toolchain != new.toolchain,
        platform_changed=old.platform != new.platform,
    )
