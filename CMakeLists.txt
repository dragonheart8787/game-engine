cmake_minimum_required(VERSION 3.16)
project(game_engine LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ENGINE_VENDOR_DEPS "Use offline vendor/system dependencies only" ON)

include(FetchContent)

find_package(glm CONFIG QUIET)
find_package(nlohmann_json CONFIG QUIET)

if (NOT TARGET glm::glm)
  find_path(GLM_INCLUDE_DIR glm/glm.hpp)
  if (GLM_INCLUDE_DIR)
    add_library(glm::glm INTERFACE IMPORTED)
    set_target_properties(glm::glm PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${GLM_INCLUDE_DIR}")
  endif()
endif()

if (NOT TARGET nlohmann_json::nlohmann_json)
  find_path(NLOHMANN_JSON_INCLUDE_DIR nlohmann/json.hpp)
  if (NLOHMANN_JSON_INCLUDE_DIR)
    add_library(nlohmann_json::nlohmann_json INTERFACE IMPORTED)
    set_target_properties(nlohmann_json::nlohmann_json PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${NLOHMANN_JSON_INCLUDE_DIR}")
  endif()
endif()

if (NOT glm_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/third_party/glm/CMakeLists.txt")
  add_subdirectory(third_party/glm)
endif()
if (NOT nlohmann_json_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/third_party/json/CMakeLists.txt")
  add_subdirectory(third_party/json)
endif()

set(_missing_vendor_deps "")
if (NOT TARGET glm::glm)
  list(APPEND _missing_vendor_deps "glm (expected: third_party/glm)")
endif()
if (NOT TARGET nlohmann_json::nlohmann_json)
  list(APPEND _missing_vendor_deps "nlohmann_json (expected: third_party/json)")
endif()

if (ENGINE_VENDOR_DEPS)
  if (_missing_vendor_deps)
    string(JOIN ", " _missing_vendor_deps_text ${_missing_vendor_deps})
    message(FATAL_ERROR
      "ENGINE_VENDOR_DEPS=ON but required dependencies are missing: ${_missing_vendor_deps_text}\n"
      "Fix options:\n"
      "  1) git submodule update --init --recursive\n"
      "  2) ./scripts/fetch_deps.sh --vendor-dir third_party --glm 0.9.9.8 --json v3.11.3\n"
      "  3) (Windows) ./scripts/fetch_deps.ps1 -VendorDir third_party -Glm 0.9.9.8 -Json v3.11.3\n"
      "  4) install system packages that provide glm + nlohmann_json config packages")
  endif()
else()
  if (NOT TARGET glm::glm)
    FetchContent_Declare(
      glm
      GIT_REPOSITORY https://github.com/g-truc/glm.git
      GIT_TAG 0.9.9.8
    )
    FetchContent_MakeAvailable(glm)
  endif()

  if (NOT TARGET nlohmann_json::nlohmann_json)
    FetchContent_Declare(
      nlohmann_json
      GIT_REPOSITORY https://github.com/nlohmann/json.git
      GIT_TAG v3.11.3
    )
    FetchContent_MakeAvailable(nlohmann_json)
  endif()
endif()

find_package(SDL2 CONFIG QUIET)
if (NOT SDL2_FOUND)
  find_package(SDL2 REQUIRED)
endif()

find_package(OpenGL REQUIRED COMPONENTS GLES2)
if (NOT TARGET OpenGL::GLES3)
  add_library(OpenGL::GLES3 INTERFACE IMPORTED)
  target_link_libraries(OpenGL::GLES3 INTERFACE OpenGL::GLES2)
endif()

enable_testing()

add_subdirectory(engine)
add_subdirectory(apps/demo_thirdperson)
add_subdirectory(tools/asset_packer)
add_subdirectory(tools/validate_assets)
add_subdirectory(tools/replay_diff)
add_subdirectory(tests/worldstate_tests)
add_subdirectory(tests/render_contract_tests)

add_subdirectory(tests/platform_runtime_tests)
