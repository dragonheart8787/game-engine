cmake_minimum_required(VERSION 3.16)
project(game_engine_platform LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(game_engine_core STATIC
  engine/src/core.cpp
)

target_include_directories(game_engine_core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/engine/include
)

add_executable(game_engine_smoke
  apps/smoke/main.cpp
)
target_link_libraries(game_engine_smoke PRIVATE game_engine_core)

enable_testing()
add_test(NAME engine_smoke COMMAND game_engine_smoke)
