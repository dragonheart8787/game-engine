"""Validation helpers for plugin payloads."""

from __future__ import annotations

from .contracts import PluginManifest


def validate_plugin_manifest(manifest: PluginManifest) -> list[str]:
    errors: list[str] = []
    for field in ("name", "version", "engine_version", "entrypoint"):
        if not getattr(manifest, field):
            errors.append(f"{field} is required")
    if not manifest.capabilities:
        errors.append("at least one capability is required")
    return errors
