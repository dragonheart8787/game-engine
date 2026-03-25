import json

from game_engine.cli import main, render_report, CliReport
from game_engine.editor.contracts import ImportRequest, SceneAsset
from game_engine.editor.history import EditorHistory
from game_engine.editor.prefab import Prefab, deserialize_prefab, serialize_prefab
from game_engine.editor.validators import validate_import_request, validate_scene_asset
from game_engine.pipeline.bundler import build_bundle_output
from game_engine.pipeline.dependency_graph import DependencyGraph
from game_engine.pipeline.migrations import PipelineState, migrate_pipeline_state, verify_pipeline_state
from game_engine.pipeline.rebuild import compare_manifests
from game_engine.pipeline.schema import BuildManifest


class _CounterCommand:
    def __init__(self, target: dict[str, int], delta: int, name: str) -> None:
        self.target = target
        self.delta = delta
        self.name = name

    def execute(self) -> None:
        self.target["value"] += self.delta

    def undo(self) -> None:
        self.target["value"] -= self.delta


def test_prefab_roundtrip() -> None:
    prefab = Prefab(prefab_id="hero", version=1, components={"transform": {"x": 1}})
    restored = deserialize_prefab(serialize_prefab(prefab))
    assert restored == prefab


def test_editor_history_execute_undo_redo() -> None:
    value = {"value": 0}
    history = EditorHistory()
    command = _CounterCommand(value, 2, "inc")

    history.execute(command)
    assert value["value"] == 2
    assert history.undo() == "inc"
    assert value["value"] == 0
    assert history.redo() == "inc"
    assert value["value"] == 2


def test_editor_validators() -> None:
    scene_issues = validate_scene_asset(SceneAsset(scene_guid="", version=0, root_entities=["a", "a"]))
    import_issues = validate_import_request(
        ImportRequest(source_path="", source_hash="x", settings_hash="")
    )
    assert len(scene_issues) == 3
    assert len(import_issues) == 3


def test_dependency_graph_topology() -> None:
    graph = DependencyGraph()
    graph.add_dependency("material", "texture")
    graph.add_dependency("scene", "material")

    order = graph.topological_order()
    assert order.index("texture") < order.index("material") < order.index("scene")


def test_dependency_graph_cycle_raises() -> None:
    graph = DependencyGraph()
    graph.add_dependency("a", "b")
    graph.add_dependency("b", "a")
    try:
        graph.topological_order()
        assert False, "Expected ValueError for cycle"
    except ValueError:
        pass


def test_pipeline_migration_and_verify() -> None:
    v1 = PipelineState(schema_version=1, records=[{"guid": "a", "hash": "1"}])
    v2 = migrate_pipeline_state(v1, target_version=2)
    assert v2.schema_version == 2
    assert v2.records[0]["compression"] == "none"
    assert verify_pipeline_state(v2) == []


def test_bundler_output_structure() -> None:
    bundle = build_bundle_output("base", ["a", "b", "c"], chunk_size=2)
    assert bundle.header.chunk_count == 2
    assert bundle.index.location_by_record["c"] == 1


def test_rebuild_report_comparison() -> None:
    old = BuildManifest("old", "tc", "pc", ["a", "b"])
    new = BuildManifest("new", "tc", "pc", ["b", "c"])
    report = compare_manifests(old, new)
    assert report.added_hashes == ["c"]
    assert report.removed_hashes == ["a"]


def test_cli_json_report_with_rebuild() -> None:
    report = CliReport(pillars=["p"], milestones=["m"], rebuild_report={"added_hashes": ["x"], "removed_hashes": []})
    output = render_report(report, "json")
    assert json.loads(output)["rebuild_report"]["added_hashes"] == ["x"]
    assert main(["--format", "json", "--old-input-hashes", "a", "--new-input-hashes", "a,b"]) == 0
