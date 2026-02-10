add_library(engine_core
  core/App.cpp
  core/Platform.cpp
  core/Log.cpp
  core/JobSystem.cpp
  core/Config.cpp
)

target_include_directories(engine_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if (TARGET SDL2::SDL2)
  target_link_libraries(engine_core PUBLIC SDL2::SDL2 OpenGL::GLES3)
else()
  target_include_directories(engine_core PUBLIC ${SDL2_INCLUDE_DIRS})
  target_link_libraries(engine_core PUBLIC ${SDL2_LIBRARIES} OpenGL::GLES3)
endif()

target_compile_definitions(engine_core PUBLIC SDL_MAIN_HANDLED)

add_library(engine_math
  math/Math.cpp
  math/Random.cpp
  math/DeterministicRng.cpp
)

target_include_directories(engine_math PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(engine_math PUBLIC glm::glm)

add_library(engine_ecs
  ecs/World.cpp
)

target_include_directories(engine_ecs PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_library(engine_world
  world/WorldState.cpp
  world/WorldDelta.cpp
  world/WorldStore.cpp
  world/WorldHasher.cpp
)

target_include_directories(engine_world PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(engine_world PUBLIC nlohmann_json::nlohmann_json engine_math)

add_library(engine_render
  render/Renderer.cpp
  render/CameraSystem.cpp
)

target_include_directories(engine_render PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(engine_render PUBLIC OpenGL::GLES3 engine_math)

add_library(engine_input
  input/InputSystem.cpp
)

target_include_directories(engine_input PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_library(engine_ability
  ability/AbilityGraph.cpp
  ability/AbilityRuntime.cpp
)

target_include_directories(engine_ability PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(engine_ability PUBLIC nlohmann_json::nlohmann_json engine_math)

add_library(engine_story
  story/Director.cpp
  story/IdentityOverride.cpp
)

target_include_directories(engine_story PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(engine_story PUBLIC nlohmann_json::nlohmann_json engine_world)

add_library(engine_procgen
  procgen/WorldGenerator.cpp
)

target_include_directories(engine_procgen PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(engine_procgen PUBLIC engine_world engine_math)

add_library(engine_debug
  debug/DebugOverlay.cpp
)

target_include_directories(engine_debug PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(engine_debug PUBLIC engine_core)

add_library(engine_audio
  audio/AudioSystem.cpp
)

target_include_directories(engine_audio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_library(engine_physics
  physics/PhysicsSystem.cpp
)

target_include_directories(engine_physics PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(engine_physics PUBLIC engine_math)

add_library(engine_ai
  ai/BehaviorSystem.cpp
)

target_include_directories(engine_ai PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_library(engine_net
  net/Replication.cpp
)

target_include_directories(engine_net PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

add_library(engine_vfx
  vfx/VfxSystem.cpp
)

target_include_directories(engine_vfx PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(engine_vfx PUBLIC engine_render)

add_library(engine_tools
  tools/AssetIO.cpp
)

target_include_directories(engine_tools PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(engine_tools PUBLIC nlohmann_json::nlohmann_json)

add_library(engine_platform
  platform/PlatformStubs.cpp
)

target_include_directories(engine_platform PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
