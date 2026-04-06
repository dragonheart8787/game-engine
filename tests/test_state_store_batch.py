from game_engine.services.state_store import StateStore


def test_put_many_versions_monotonic() -> None:
    store = StateStore()
    store.put("a", 1)
    out = store.put_many({"a": 2, "b": 3})
    assert out["a"].version == 2
    assert out["b"].version == 1
    snap = store.snapshot()
    assert snap["a"].value == 2
    assert snap["b"].value == 3
