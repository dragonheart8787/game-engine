from game_engine.ops.health import HealthRegistry, HealthStatus
from game_engine.services.lifecycle import GracefulShutdown
from game_engine.services.replication import ReplicationService
from game_engine.services.state_store import StateStore


def test_replication_service_updates_state_and_notifies_subscribers() -> None:
    store = StateStore()
    replication = ReplicationService(store)
    seen: list[tuple[str, int]] = []
    replication.subscribe(lambda evt: seen.append((evt.key, evt.version)))

    replication.replicate("player:1", {"hp": 100})
    replication.replicate("player:1", {"hp": 75})

    record = store.get("player:1")
    assert record is not None
    assert record.version == 2
    assert record.value["hp"] == 75
    assert seen == [("player:1", 1), ("player:1", 2)]


def test_state_store_compare_and_set_guards_versions() -> None:
    store = StateStore()
    first = store.compare_and_set("match:1", 0, {"state": "queued"})
    assert first.version == 1

    second = store.compare_and_set("match:1", 1, {"state": "running"})
    assert second.version == 2

    try:
        store.compare_and_set("match:1", 1, {"state": "done"})
        assert False, "expected version mismatch"
    except ValueError:
        pass


def test_health_registry_aggregates_service_checks() -> None:
    registry = HealthRegistry()
    registry.register("db", lambda: HealthStatus.OK)
    registry.register("replication", lambda: HealthStatus.DEGRADED)
    registry.register("matchmaker", lambda: HealthStatus.FAIL)

    report = registry.run()
    assert report.overall == HealthStatus.FAIL
    assert len(report.issues) == 2


def test_graceful_shutdown_runs_hooks_in_reverse_order() -> None:
    order: list[str] = []
    shutdown = GracefulShutdown()

    shutdown.register(lambda: order.append("first"))
    shutdown.register(lambda: order.append("second"))
    shutdown.shutdown()

    assert order == ["second", "first"]
    assert shutdown.closed
