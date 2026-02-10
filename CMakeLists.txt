cmake_minimum_required(VERSION 3.16)
project(game_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ENGINE_VENDOR_DEPS "Use offline vendor/system dependencies only" ON)

include(FetchContent)

find_package(glm CONFIG QUIET)
find_package(nlohmann_json CONFIG QUIET)

if (NOT glm_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/third_party/glm/CMakeLists.txt")
  add_subdirectory(third_party/glm)
endif()
if (NOT nlohmann_json_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/third_party/json/CMakeLists.txt")
  add_subdirectory(third_party/json)
endif()

if (NOT TARGET glm::glm)
  if (ENGINE_VENDOR_DEPS)
    message(FATAL_ERROR "glm not found. Provide system package or third_party/glm when ENGINE_VENDOR_DEPS=ON")
  endif()
  FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 0.9.9.8
  )
  FetchContent_MakeAvailable(glm)
endif()

if (NOT TARGET nlohmann_json::nlohmann_json)
  if (ENGINE_VENDOR_DEPS)
    message(FATAL_ERROR "nlohmann_json not found. Provide system package or third_party/json when ENGINE_VENDOR_DEPS=ON")
  endif()
  FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
  )
  FetchContent_MakeAvailable(nlohmann_json)
endif()

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
add_subdirectory(tools/replay_diff)
add_subdirectory(tests/worldstate_tests)
