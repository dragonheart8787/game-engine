cmake_minimum_required(VERSION 3.16)
project(game_engine_ecs LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(ecs
    engine/ecs/World.cpp
)
target_include_directories(ecs PUBLIC .)

add_executable(ecs_tests tests/ecs_tests.cpp)
target_link_libraries(ecs_tests PRIVATE ecs)
