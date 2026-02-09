cmake_minimum_required(VERSION 3.16)
project(game_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(SDL2 REQUIRED)

add_executable(game_engine
    src/main.cpp
    src/engine/Engine.cpp
    src/engine/Engine.h
    src/engine/Renderer.cpp
    src/engine/Renderer.h
)

target_include_directories(game_engine PRIVATE ${SDL2_INCLUDE_DIRS} src)
target_link_libraries(game_engine PRIVATE ${SDL2_LIBRARIES})
