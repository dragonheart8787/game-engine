# Engine Loop Contract

## Lifecycle
- `initialize(context)`
- `load_startup_scene(scene_id)`
- `tick(frame_time)`
- `render(frame_graph)`
- `shutdown(reason)`

## Subsystem Registration Order
1. Logging/Telemetry
2. Asset
3. Render
4. Scene
5. Physics
6. Audio
7. Input
8. UI
9. Scripting
10. Network

## Error Policy
- Initialization failures are fatal.
- Runtime subsystem failures are isolated and reported.
