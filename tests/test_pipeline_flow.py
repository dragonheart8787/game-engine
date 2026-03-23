from pathlib import Path

from game_engine.pipeline.bundler import write_bundle
from game_engine.pipeline.importer import import_asset
from game_engine.pipeline.manifest_builder import manifest_hash
from game_engine.pipeline.schema import BuildManifest, BundleMetadata


def test_manifest_hash_is_deterministic() -> None:
    manifest_a = BuildManifest(
        commit_sha="abc",
        toolchain="py311",
        platform="linux",
        input_hashes=["2", "1"],
    )
    manifest_b = BuildManifest(
        commit_sha="abc",
        toolchain="py311",
        platform="linux",
        input_hashes=["1", "2"],
    )
    assert manifest_hash(manifest_a) == manifest_hash(manifest_b)


def test_import_and_bundle(tmp_path: Path) -> None:
    source = tmp_path / "asset.txt"
    source.write_text("content", encoding="utf-8")

    imported = import_asset(str(source), {"quality": "high"})
    metadata = BundleMetadata(
        bundle_id="main",
        semver="0.1.0",
        platform="linux",
        compression_mode="none",
        encryption_profile="none",
        content_manifest_hash=imported["source_hash"],
    )
    bundle = write_bundle(str(tmp_path), metadata, imported["artifact_refs"])
    assert Path(bundle).exists()
