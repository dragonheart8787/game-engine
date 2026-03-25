"""Bundle output structure generation (header/index/chunks)."""

from __future__ import annotations

from dataclasses import dataclass


@dataclass(slots=True)
class BundleHeader:
    bundle_id: str
    chunk_count: int
    record_count: int


@dataclass(slots=True)
class BundleChunk:
    chunk_id: int
    records: list[str]


@dataclass(slots=True)
class BundleIndex:
    location_by_record: dict[str, int]


@dataclass(slots=True)
class BundleOutput:
    header: BundleHeader
    index: BundleIndex
    chunks: list[BundleChunk]


def build_bundle_output(bundle_id: str, records: list[str], chunk_size: int = 2) -> BundleOutput:
    if chunk_size <= 0:
        raise ValueError("chunk_size must be > 0")

    chunks: list[BundleChunk] = []
    index: dict[str, int] = {}
    for i in range(0, len(records), chunk_size):
        chunk_records = records[i : i + chunk_size]
        chunk_id = len(chunks)
        chunks.append(BundleChunk(chunk_id=chunk_id, records=chunk_records))
        for record in chunk_records:
            index[record] = chunk_id

    return BundleOutput(
        header=BundleHeader(bundle_id=bundle_id, chunk_count=len(chunks), record_count=len(records)),
        index=BundleIndex(location_by_record=index),
        chunks=chunks,
    )
