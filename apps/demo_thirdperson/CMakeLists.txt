add_executable(demo_thirdperson
  main.cpp
  DemoGame.cpp
  DemoGame.h
)

target_include_directories(demo_thirdperson PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(demo_thirdperson PRIVATE
  engine_core
  engine_math
  engine_ecs
  engine_world
  engine_render
  engine_input
  engine_ability
  engine_story
  engine_procgen
  engine_debug
  engine_physics
  engine_vfx
  engine_tools
)
