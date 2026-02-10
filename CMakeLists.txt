cmake_minimum_required(VERSION 3.16)
project(game_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm.git
  GIT_TAG 0.9.9.8
)
FetchContent_MakeAvailable(glm)

FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

find_package(SDL2 CONFIG QUIET)
if (NOT SDL2_FOUND)
  find_package(SDL2 REQUIRED)
endif()

find_package(OpenGL REQUIRED COMPONENTS GLES3)

enable_testing()

add_subdirectory(engine)
add_subdirectory(apps/demo_thirdperson)
add_subdirectory(tools/asset_packer)
add_subdirectory(tools/validate_assets)
add_subdirectory(tests/worldstate_tests)
