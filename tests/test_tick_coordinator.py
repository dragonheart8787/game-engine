from game_engine.runtime.ecs import SystemRegistry
from game_engine.runtime.systems import UiState, register_standard_systems
from game_engine.runtime.tick_coordinator import TickCoordinator


def test_tick_coordinator_delegates_to_registry() -> None:
    coord = TickCoordinator(world={})
    seen: list[float] = []

    def capture(dt: float, world: dict[str, object]) -> None:
        seen.append(dt)

    coord.registry.register("probe", capture)
    coord.tick(0.05)
    assert seen == [0.05]


def test_register_standard_systems_runs_ui() -> None:
    reg = SystemRegistry()
    register_standard_systems(reg)
    ui = UiState(dirty=True)
    coord = TickCoordinator(world={"ui": ui}, registry=reg)
    coord.tick(0.0)
    assert ui.layout_passes == 1
    assert not ui.dirty
