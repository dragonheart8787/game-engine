"""Pipeline schemas and reproducibility helpers."""

from dataclasses import dataclass


@dataclass(slots=True)
class AssetRecord:
    guid: str
    asset_type: str
    source: str
    version: int
    content_hash: str


@dataclass(slots=True)
class BuildManifest:
    commit_sha: str
    toolchain: str
    platform: str
    input_hashes: list[str]


@dataclass(slots=True)
class BundleMetadata:
    bundle_id: str
    semver: str
    platform: str
    compression_mode: str
    encryption_profile: str
    content_manifest_hash: str
