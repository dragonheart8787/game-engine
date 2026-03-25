import json

from game_engine.cli import main, render_report, CliReport
from game_engine.ops.benchmark import run_benchmark
from game_engine.ops.crash import (
    CrashPayload,
    aggregate_crashes,
    calculate_stack_fingerprint,
)
from game_engine.ops.observability import MetricStore, dump_metrics
from game_engine.pipeline.patch_manifest import PatchManifest, apply_rollback_map, validate_rollback_map


def test_crash_fingerprint_and_aggregation() -> None:
    stack = ["Engine::Tick", "Render::Frame"]
    fingerprint = calculate_stack_fingerprint(stack, "sym-v1")
    payload = CrashPayload(
        build_id="dev",
        platform="linux",
        stack_fingerprint=fingerprint,
        symbol_version="sym-v1",
        session_metadata={"region": "us"},
    )
    out = aggregate_crashes([payload, payload])
    assert list(out.values()) == [2]


def test_patch_manifest_rollback_map() -> None:
    manifest = PatchManifest(
        base_manifest_hash="base",
        patch_manifest_hash="patch",
        changed_assets={"a": "newa", "b": "newb"},
    )
    issues = validate_rollback_map(manifest, {"a": "olda", "c": "oldc"})
    assert "unknown asset 'c'" in issues

    rolled = apply_rollback_map(manifest, {"a": "olda", "c": "oldc"})
    assert rolled.changed_assets["a"] == "olda"
    assert rolled.changed_assets["b"] == "newb"


def test_observability_dump_and_cli_metrics() -> None:
    metrics = MetricStore()
    metrics.inc("frames", 2)
    metrics.set_gauge("fps", 120.0)
    metrics.emit("frame", stage="end")
    payload = json.loads(dump_metrics(metrics))
    assert payload["counters"]["frames"] == 2

    report = CliReport(pillars=["p"], milestones=["m"], metrics_dump='{"ok":1}')
    rendered = render_report(report, "text")
    assert "Metrics:" in rendered
    assert main(["--dump-metrics", "--format", "json"]) == 0


def test_benchmark_supports_multiple_rounds() -> None:
    total = {"n": 0}

    def work() -> None:
        total["n"] += 1

    report = run_benchmark(work, rounds=5)
    assert report.rounds == 5
    assert len(report.durations_ms) == 5
    assert total["n"] == 5
    assert report.max_ms >= report.min_ms
