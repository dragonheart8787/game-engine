from game_engine.runtime.animation import blend
from game_engine.runtime.audio import AudioTrack, mix
from game_engine.runtime.ecs import World
from game_engine.runtime.physics import RigidBody, step
from game_engine.runtime.render_backends import RenderBackend, create_backend
from game_engine.runtime.ui import Widget, vertical_layout


def test_render_backend_encoders() -> None:
    for backend in (RenderBackend.VULKAN, RenderBackend.DIRECTX12, RenderBackend.METAL):
        frame = create_backend(backend).encode_frame(frame_index=1, draw_calls=3)
        assert frame.backend is backend
        assert frame.draw_calls == 3
        assert len(frame.command_buffer) == 3


def test_ecs_physics_audio_ui_animation() -> None:
    world = World()
    world.create_entity("player")
    world.add_component("player", "transform", {"x": 0.0, "y": 1.0, "z": 0.0})
    assert len(world.query("transform")) == 1

    body = step(RigidBody(position_y=10.0, velocity_y=0.0), dt_seconds=0.016)
    assert body.position_y < 10.0

    stereo = mix([AudioTrack(name="bgm", volume=0.8, pan=0.0)])
    assert stereo["left"] == stereo["right"]

    layout = vertical_layout([Widget("w1", 100, 20), Widget("w2", 100, 20)], spacing=10)
    assert layout["w2"][1] == 30

    assert blend(0.0, 10.0, 0.5) == 5.0
