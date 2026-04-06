cmake_minimum_required(VERSION 3.16)
project(game_engine_platform LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# MSVC: treat sources/headers as UTF-8 (avoids C4819 when comments contain non-CP950 chars).
if(MSVC)
  add_compile_options(/utf-8)
endif()

set(WB_ENGINE_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/engine/include")

option(WEAVEBOUND_WITH_JOLT "Integrate Jolt Physics (FetchContent)" ON)

set(WB_HAS_JOLT 0)
if(WEAVEBOUND_WITH_JOLT)
  include(FetchContent)
  set(OVERRIDE_CXX_FLAGS OFF CACHE BOOL "" FORCE)
  set(CROSS_PLATFORM_DETERMINISTIC OFF CACHE BOOL "" FORCE)
  set(INTERPROCEDURAL_OPTIMIZATION OFF CACHE BOOL "" FORCE)
  set(ENABLE_ALL_WARNINGS OFF CACHE BOOL "" FORCE)
  set(USE_STATIC_MSVC_RUNTIME_LIBRARY OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(
    JoltPhysics
    GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git
    GIT_TAG v5.2.0
    SOURCE_SUBDIR Build
  )
  FetchContent_MakeAvailable(JoltPhysics)
  if(TARGET Jolt)
    set_target_properties(Jolt PROPERTIES DISABLE_PRECOMPILE_HEADERS ON)
    if(MSVC)
      target_compile_options(Jolt PRIVATE /wd4828)
    endif()
    set(WB_HAS_JOLT 1)
    message(STATUS "WeaveBound: Jolt Physics enabled")
  else()
    message(WARNING "WeaveBound: Jolt target not found; physics stub only")
  endif()
endif()

# --- WeaveBound platform ---
set(WB_PLATFORM_SOURCES
  engine/src/weavebound/platform/clock_std.cpp
  engine/src/weavebound/platform/window_stub.cpp
  engine/src/weavebound/platform/job_inline.cpp
  engine/src/weavebound/platform/vfs_disk.cpp
)
if(WIN32)
  list(APPEND WB_PLATFORM_SOURCES engine/src/weavebound/platform/win32/window_win32.cpp)
endif()

add_library(weavebound_platform STATIC ${WB_PLATFORM_SOURCES})
target_include_directories(weavebound_platform PUBLIC "${WB_ENGINE_INCLUDE}")
if(WIN32)
  target_link_libraries(weavebound_platform PUBLIC user32 gdi32)
endif()

# Dear ImGui（Win32 訊息轉發；Vulkan 後端在啟用 RHI Vulkan 時一併編譯）
if(WIN32)
  include(FetchContent)
  FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.0
  )
  FetchContent_MakeAvailable(imgui)
  set(WB_IMGUI_SRC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
  )
endif()

# --- Vulkan（僅 Win64 + 偵測到 Vulkan SDK）---
find_package(Vulkan QUIET)
if(Vulkan_FOUND AND WIN32)
  set(WB_HAS_VULKAN 1)
else()
  set(WB_HAS_VULKAN 0)
endif()

if(WIN32)
  if(WB_HAS_VULKAN)
    list(APPEND WB_IMGUI_SRC ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp)
  endif()
  add_library(weavebound_imgui STATIC ${WB_IMGUI_SRC})
  target_include_directories(weavebound_imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
  target_link_libraries(weavebound_imgui PUBLIC user32)
  if(WB_HAS_VULKAN)
    target_link_libraries(weavebound_imgui PUBLIC Vulkan::Vulkan)
  endif()
  target_link_libraries(weavebound_platform PRIVATE weavebound_imgui)
endif()

set(WB_RHI_SOURCES
  engine/src/weavebound/rhi/device.cpp
  engine/src/weavebound/rhi/metal/device_metal_stub.cpp
)
if(WB_HAS_VULKAN)
  list(APPEND WB_RHI_SOURCES
    engine/src/weavebound/rhi/vulkan/device_vulkan.cpp
    engine/src/weavebound/rhi/vulkan/image_ops.cpp
    engine/src/weavebound/rhi/vulkan/rg_executor_vulkan.cpp
    engine/src/weavebound/rhi/vulkan/lit_demo_vulkan.cpp
  )
endif()

add_library(weavebound_rhi STATIC ${WB_RHI_SOURCES})
target_include_directories(weavebound_rhi PUBLIC "${WB_ENGINE_INCLUDE}")
target_link_libraries(weavebound_rhi PUBLIC weavebound_platform)
target_compile_definitions(weavebound_rhi PRIVATE "WB_HAS_VULKAN=${WB_HAS_VULKAN}")
if(WB_HAS_VULKAN)
  target_link_libraries(weavebound_rhi PRIVATE Vulkan::Vulkan weavebound_imgui)
  target_compile_definitions(weavebound_rhi PRIVATE VK_USE_PLATFORM_WIN32_KHR)
endif()

# --- Shader SPIR-V（glslc 來自 Vulkan SDK，或 engine/shaders/baked/ 預編譯）---
set(WB_SHADER_SRC "${CMAKE_CURRENT_SOURCE_DIR}/engine/shaders")
set(WB_SPV_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")
if(WB_HAS_VULKAN)
  if(DEFINED ENV{VULKAN_SDK})
    file(TO_CMAKE_PATH "$ENV{VULKAN_SDK}" _VK_SDK)
    find_program(GLSLC NAMES glslc HINTS "${_VK_SDK}/Bin" "${_VK_SDK}/bin" NO_DEFAULT_PATH)
  endif()
  if(NOT GLSLC)
    find_program(GLSLC glslc)
  endif()
  set(WB_VERT_SPV "${WB_SPV_GEN_DIR}/triangle.vert.spv")
  set(WB_FRAG_SPV "${WB_SPV_GEN_DIR}/triangle.frag.spv")
  set(WB_COMP_SPV "${WB_SPV_GEN_DIR}/trivial.comp.spv")
  set(WB_SHADOW_VS_SPV "${WB_SPV_GEN_DIR}/shadow_depth.vert.spv")
  set(WB_SHADOW_FS_SPV "${WB_SPV_GEN_DIR}/shadow_depth.frag.spv")
  set(WB_LIT_VS_SPV "${WB_SPV_GEN_DIR}/lit_mesh.vert.spv")
  set(WB_LIT_FS_SPV "${WB_SPV_GEN_DIR}/lit_mesh.frag.spv")
  set(WB_FSQ_VS_SPV "${WB_SPV_GEN_DIR}/fullscreen.vert.spv")
  set(WB_COMP_FS_SPV "${WB_SPV_GEN_DIR}/composite.frag.spv")
  set(WB_ALL_SPV
    ${WB_VERT_SPV} ${WB_FRAG_SPV} ${WB_COMP_SPV}
    ${WB_SHADOW_VS_SPV} ${WB_SHADOW_FS_SPV} ${WB_LIT_VS_SPV} ${WB_LIT_FS_SPV} ${WB_FSQ_VS_SPV} ${WB_COMP_FS_SPV})
  set(WB_SHADERS_INCLUDE_COMP 0)
  if(GLSLC)
    set(WB_SHADERS_INCLUDE_COMP 1)
    message(STATUS "glslc: ${GLSLC}")
    add_custom_command(
      OUTPUT ${WB_ALL_SPV}
      COMMAND ${CMAKE_COMMAND} -E make_directory "${WB_SPV_GEN_DIR}"
      COMMAND "${GLSLC}" -o "${WB_VERT_SPV}" "${WB_SHADER_SRC}/triangle.vert"
      COMMAND "${GLSLC}" -o "${WB_FRAG_SPV}" "${WB_SHADER_SRC}/triangle.frag"
      COMMAND "${GLSLC}" -o "${WB_COMP_SPV}" "${WB_SHADER_SRC}/trivial.comp"
      COMMAND "${GLSLC}" -o "${WB_SHADOW_VS_SPV}" "${WB_SHADER_SRC}/shadow_depth.vert"
      COMMAND "${GLSLC}" -o "${WB_SHADOW_FS_SPV}" "${WB_SHADER_SRC}/shadow_depth.frag"
      COMMAND "${GLSLC}" -o "${WB_LIT_VS_SPV}" "${WB_SHADER_SRC}/lit_mesh.vert"
      COMMAND "${GLSLC}" -o "${WB_LIT_FS_SPV}" "${WB_SHADER_SRC}/lit_mesh.frag"
      COMMAND "${GLSLC}" -o "${WB_FSQ_VS_SPV}" "${WB_SHADER_SRC}/fullscreen.vert"
      COMMAND "${GLSLC}" -o "${WB_COMP_FS_SPV}" "${WB_SHADER_SRC}/composite.frag"
      DEPENDS
        "${WB_SHADER_SRC}/triangle.vert" "${WB_SHADER_SRC}/triangle.frag" "${WB_SHADER_SRC}/trivial.comp"
        "${WB_SHADER_SRC}/shadow_depth.vert" "${WB_SHADER_SRC}/shadow_depth.frag"
        "${WB_SHADER_SRC}/lit_mesh.vert" "${WB_SHADER_SRC}/lit_mesh.frag"
        "${WB_SHADER_SRC}/fullscreen.vert" "${WB_SHADER_SRC}/composite.frag"
      VERBATIM
      COMMENT "Compile GLSL (glslc)"
    )
    add_custom_target(weavebound_shaders DEPENDS ${WB_ALL_SPV})
    add_dependencies(weavebound_rhi weavebound_shaders)
  elseif(EXISTS "${WB_SHADER_SRC}/baked/triangle.vert.spv" AND EXISTS "${WB_SHADER_SRC}/baked/triangle.frag.spv" AND EXISTS "${WB_SHADER_SRC}/baked/trivial.comp.spv")
    set(WB_SHADERS_INCLUDE_COMP 1)
    message(STATUS "Using baked SPIR-V from engine/shaders/baked/ (incl. trivial.comp)")
    add_custom_command(
      OUTPUT ${WB_VERT_SPV} ${WB_FRAG_SPV} ${WB_COMP_SPV}
      COMMAND ${CMAKE_COMMAND} -E make_directory "${WB_SPV_GEN_DIR}"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${WB_SHADER_SRC}/baked/triangle.vert.spv" "${WB_VERT_SPV}"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${WB_SHADER_SRC}/baked/triangle.frag.spv" "${WB_FRAG_SPV}"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${WB_SHADER_SRC}/baked/trivial.comp.spv" "${WB_COMP_SPV}"
      DEPENDS "${WB_SHADER_SRC}/baked/triangle.vert.spv" "${WB_SHADER_SRC}/baked/triangle.frag.spv" "${WB_SHADER_SRC}/baked/trivial.comp.spv"
      VERBATIM
    )
    add_custom_target(weavebound_shaders DEPENDS ${WB_VERT_SPV} ${WB_FRAG_SPV} ${WB_COMP_SPV})
    add_dependencies(weavebound_rhi weavebound_shaders)
  elseif(EXISTS "${WB_SHADER_SRC}/baked/triangle.vert.spv" AND EXISTS "${WB_SHADER_SRC}/baked/triangle.frag.spv")
    message(STATUS "Using baked SPIR-V from engine/shaders/baked/ (no trivial.comp: compute optional)")
    add_custom_command(
      OUTPUT ${WB_VERT_SPV} ${WB_FRAG_SPV}
      COMMAND ${CMAKE_COMMAND} -E make_directory "${WB_SPV_GEN_DIR}"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${WB_SHADER_SRC}/baked/triangle.vert.spv" "${WB_VERT_SPV}"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${WB_SHADER_SRC}/baked/triangle.frag.spv" "${WB_FRAG_SPV}"
      DEPENDS "${WB_SHADER_SRC}/baked/triangle.vert.spv" "${WB_SHADER_SRC}/baked/triangle.frag.spv"
      VERBATIM
    )
    add_custom_target(weavebound_shaders DEPENDS ${WB_VERT_SPV} ${WB_FRAG_SPV})
    add_dependencies(weavebound_rhi weavebound_shaders)
  else()
    message(STATUS "No glslc / no baked SPIR-V: triangle overlay disabled (run scripts/bootstrap/compile_shaders)")
  endif()
endif()

add_library(weavebound_render_graph STATIC
  engine/src/weavebound/render_graph/builder.cpp
  engine/src/weavebound/render_graph/compiler.cpp
)
target_include_directories(weavebound_render_graph PUBLIC "${WB_ENGINE_INCLUDE}")

add_library(weavebound_net INTERFACE)
target_include_directories(weavebound_net INTERFACE "${WB_ENGINE_INCLUDE}")

add_library(weavebound_ecs STATIC
  engine/src/weavebound/ecs/registry.cpp
)
target_include_directories(weavebound_ecs PUBLIC "${WB_ENGINE_INCLUDE}")

add_library(weavebound_asset STATIC
  engine/src/weavebound/asset/wbpak.cpp
  engine/src/weavebound/asset/vfs_pak.cpp
)
target_include_directories(weavebound_asset PUBLIC "${WB_ENGINE_INCLUDE}")
target_link_libraries(weavebound_asset PUBLIC weavebound_platform)

add_library(weavebound_scene STATIC
  engine/src/weavebound/scene/scene_binary.cpp
)
target_include_directories(weavebound_scene PUBLIC "${WB_ENGINE_INCLUDE}")
target_link_libraries(weavebound_scene PUBLIC weavebound_ecs)

add_library(weavebound_observability STATIC
  engine/src/weavebound/observability/profiler_noop.cpp
  engine/src/weavebound/observability/logger_stderr.cpp
  engine/src/weavebound/observability/frame_meter.cpp
)
target_include_directories(weavebound_observability PUBLIC "${WB_ENGINE_INCLUDE}")

add_library(weavebound_integrations STATIC
  engine/src/weavebound/integrations/minimal_stub.cpp
)
target_include_directories(weavebound_integrations PUBLIC "${WB_ENGINE_INCLUDE}")
if(WB_HAS_JOLT)
  add_library(weavebound_physics STATIC engine/src/weavebound/physics/jolt_world.cpp)
  target_include_directories(weavebound_physics PUBLIC "${WB_ENGINE_INCLUDE}")
  target_link_libraries(weavebound_physics PUBLIC Jolt)
  target_link_libraries(weavebound_integrations PUBLIC weavebound_physics)
  target_compile_definitions(weavebound_integrations PUBLIC WEAVEBOUND_WITH_JOLT=1)
else()
  target_compile_definitions(weavebound_integrations PUBLIC WEAVEBOUND_WITH_JOLT=0)
endif()

set(WB_MINIAUDIO_DIR "${CMAKE_CURRENT_BINARY_DIR}/third_party")
set(WB_MINIAUDIO_H "${WB_MINIAUDIO_DIR}/miniaudio.h")
file(MAKE_DIRECTORY "${WB_MINIAUDIO_DIR}")
set(_wb_need_miniaudio 1)
if(EXISTS "${WB_MINIAUDIO_H}")
  file(SIZE "${WB_MINIAUDIO_H}" _wb_ma_sz)
  # 失敗或快取損毀時常留下空檔；miniaudio.h 實際為大型單檔標頭。
  if(_wb_ma_sz GREATER 800000)
    set(_wb_need_miniaudio 0)
  endif()
endif()
if(_wb_need_miniaudio)
  file(REMOVE "${WB_MINIAUDIO_H}")
  set(_wb_ma_ok 0)
  foreach(
    _wb_ma_url
    IN
    ITEMS
      "https://raw.githubusercontent.com/mackron/miniaudio/v0.11.21/miniaudio.h"
      "https://cdn.jsdelivr.net/gh/mackron/miniaudio@v0.11.21/miniaudio.h"
  )
    file(DOWNLOAD "${_wb_ma_url}" "${WB_MINIAUDIO_H}" TLS_VERIFY ON STATUS _wb_ma_dl)
    list(GET _wb_ma_dl 0 _wb_ma_code)
    if(_wb_ma_code EQUAL 0 AND EXISTS "${WB_MINIAUDIO_H}")
      file(SIZE "${WB_MINIAUDIO_H}" _wb_ma_chk)
      if(_wb_ma_chk GREATER 800000)
        set(_wb_ma_ok 1)
        message(STATUS "Fetched miniaudio.h (${_wb_ma_chk} bytes) from ${_wb_ma_url}")
        break()
      endif()
    endif()
    file(REMOVE "${WB_MINIAUDIO_H}")
  endforeach()
  if(NOT _wb_ma_ok)
    message(
      FATAL_ERROR
        "Could not download miniaudio.h (network or TLS). "
        "Download https://github.com/mackron/miniaudio/raw/v0.11.21/miniaudio.h manually to:\n  ${WB_MINIAUDIO_H}"
    )
  endif()
endif()

add_library(weavebound_audio STATIC engine/src/weavebound/audio/mixer_miniaudio.cpp)
target_include_directories(weavebound_audio PUBLIC "${WB_ENGINE_INCLUDE}" PRIVATE "${WB_MINIAUDIO_DIR}")
if(UNIX AND NOT APPLE)
  target_link_libraries(weavebound_audio PUBLIC pthread m dl)
endif()

add_library(weavebound_scripting STATIC engine/src/weavebound/scripting/lua_host_stub.cpp)
target_include_directories(weavebound_scripting PUBLIC "${WB_ENGINE_INCLUDE}")

add_library(weavebound_renderer STATIC
  engine/src/weavebound/renderer/forward_plus_phase1.cpp
)
target_include_directories(weavebound_renderer PUBLIC "${WB_ENGINE_INCLUDE}")
target_link_libraries(weavebound_renderer PUBLIC weavebound_ecs)

add_library(weavebound_engine STATIC
  engine/src/weavebound/engine/application.cpp
)
target_include_directories(weavebound_engine PUBLIC "${WB_ENGINE_INCLUDE}")
target_link_libraries(weavebound_engine PUBLIC weavebound_platform weavebound_rhi weavebound_ecs
                                                                 weavebound_observability)

add_library(weavebound_game STATIC
  engine/src/weavebound/game/save_game_v0.cpp
)
target_include_directories(weavebound_game PUBLIC "${WB_ENGINE_INCLUDE}")

add_library(game_engine_core STATIC
  engine/src/core.cpp
)
target_include_directories(game_engine_core PUBLIC "${WB_ENGINE_INCLUDE}")
target_link_libraries(game_engine_core PUBLIC
  weavebound_platform
  weavebound_rhi
  weavebound_render_graph
  weavebound_engine
  weavebound_ecs
  weavebound_asset
  weavebound_scene
  weavebound_net
  weavebound_observability
  weavebound_integrations
  weavebound_audio
  weavebound_scripting
  weavebound_renderer
)

option(WEAVEBOUND_WITH_TINYGLTF "Cook: FetchContent tinygltf for .glb mesh" ON)
if(WEAVEBOUND_WITH_TINYGLTF)
  include(FetchContent)
  FetchContent_Declare(
    tinygltf
    GIT_REPOSITORY https://github.com/syoyo/tinygltf.git
    GIT_TAG v2.8.23
  )
  FetchContent_MakeAvailable(tinygltf)
endif()

add_executable(weavebound_cook
  tools/cook/main.cpp
  tools/cook/cook_gltf.cpp
)
target_include_directories(weavebound_cook PRIVATE "${WB_ENGINE_INCLUDE}"
                                                   "${CMAKE_CURRENT_SOURCE_DIR}/tools/cook")
if(WEAVEBOUND_WITH_TINYGLTF)
  target_include_directories(weavebound_cook PRIVATE ${tinygltf_SOURCE_DIR})
  target_compile_definitions(weavebound_cook PRIVATE WEAVEBOUND_WITH_TINYGLTF=1)
endif()
if(MSVC)
  target_compile_definitions(weavebound_cook PRIVATE NOMINMAX)
endif()

add_executable(weavebound_wbpak
  tools/wbpak/main.cpp
)
target_link_libraries(weavebound_wbpak PRIVATE weavebound_asset)

add_executable(weavebound_save_game_v0_smoke
  engine/tests/save_game_v0_smoke.cpp
)
target_link_libraries(weavebound_save_game_v0_smoke PRIVATE weavebound_game)

add_executable(weavebound_game_prototype
  apps/weavebound_game_prototype/main.cpp
  apps/weavebound_game_prototype/game_flow.cpp
  apps/weavebound_game_prototype/play_session.cpp
  apps/weavebound_game_prototype/action_map.cpp
  apps/weavebound_game_prototype/ability_spec_loader.cpp
  apps/weavebound_game_prototype/prototype_settings.cpp
  apps/weavebound_game_prototype/level_spec.cpp
)
target_include_directories(weavebound_game_prototype PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/apps/weavebound_game_prototype")
target_link_libraries(weavebound_game_prototype PRIVATE game_engine_core weavebound_game)
add_custom_command(TARGET weavebound_game_prototype POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/data/ability_slice_v0.json"
    "$<TARGET_FILE_DIR:weavebound_game_prototype>/ability_slice_v0.json"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/data/level_m1.json"
    "$<TARGET_FILE_DIR:weavebound_game_prototype>/level_m1.json"
  VERBATIM)
if(WB_HAS_VULKAN AND TARGET weavebound_shaders AND DEFINED WB_ALL_SPV)
  target_compile_definitions(weavebound_game_prototype PRIVATE WB_GAME_PROTOTYPE_HAS_VULKAN=1)
  target_link_libraries(weavebound_game_prototype PRIVATE weavebound_imgui)
  add_dependencies(weavebound_game_prototype weavebound_shaders)
  set(_WB_WBGP_POST
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:weavebound_game_prototype>/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${WB_VERT_SPV}" "$<TARGET_FILE_DIR:weavebound_game_prototype>/shaders/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${WB_FRAG_SPV}" "$<TARGET_FILE_DIR:weavebound_game_prototype>/shaders/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${WB_SHADOW_VS_SPV}" "$<TARGET_FILE_DIR:weavebound_game_prototype>/shaders/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${WB_SHADOW_FS_SPV}" "$<TARGET_FILE_DIR:weavebound_game_prototype>/shaders/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${WB_LIT_VS_SPV}" "$<TARGET_FILE_DIR:weavebound_game_prototype>/shaders/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${WB_LIT_FS_SPV}" "$<TARGET_FILE_DIR:weavebound_game_prototype>/shaders/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${WB_FSQ_VS_SPV}" "$<TARGET_FILE_DIR:weavebound_game_prototype>/shaders/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${WB_COMP_FS_SPV}" "$<TARGET_FILE_DIR:weavebound_game_prototype>/shaders/"
  )
  if(WB_SHADERS_INCLUDE_COMP)
    list(APPEND _WB_WBGP_POST COMMAND ${CMAKE_COMMAND} -E copy_if_different "${WB_COMP_SPV}" "$<TARGET_FILE_DIR:weavebound_game_prototype>/shaders/")
  endif()
  add_custom_command(TARGET weavebound_game_prototype POST_BUILD ${_WB_WBGP_POST} VERBATIM)
endif()

add_executable(game_engine_smoke
  apps/smoke/main.cpp
)
target_link_libraries(game_engine_smoke PRIVATE game_engine_core)

if(WB_HAS_VULKAN AND TARGET weavebound_shaders)
  add_dependencies(game_engine_smoke weavebound_shaders)
  if(WB_SHADERS_INCLUDE_COMP)
    add_custom_command(TARGET game_engine_smoke POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:game_engine_smoke>/shaders"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${WB_VERT_SPV}" "$<TARGET_FILE_DIR:game_engine_smoke>/shaders/"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${WB_FRAG_SPV}" "$<TARGET_FILE_DIR:game_engine_smoke>/shaders/"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${WB_COMP_SPV}" "$<TARGET_FILE_DIR:game_engine_smoke>/shaders/"
      VERBATIM
    )
  else()
    add_custom_command(TARGET game_engine_smoke POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:game_engine_smoke>/shaders"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${WB_VERT_SPV}" "$<TARGET_FILE_DIR:game_engine_smoke>/shaders/"
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${WB_FRAG_SPV}" "$<TARGET_FILE_DIR:game_engine_smoke>/shaders/"
      VERBATIM
    )
  endif()
endif()

if(WB_HAS_VULKAN AND TARGET weavebound_shaders AND DEFINED WB_ALL_SPV)
  add_executable(renderer_demo apps/renderer_demo/main.cpp)
  target_link_libraries(renderer_demo PRIVATE game_engine_core weavebound_imgui)
  add_dependencies(renderer_demo weavebound_shaders)
  set(_WB_RD_POST
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:renderer_demo>/shaders"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${WB_VERT_SPV}" "$<TARGET_FILE_DIR:renderer_demo>/shaders/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${WB_FRAG_SPV}" "$<TARGET_FILE_DIR:renderer_demo>/shaders/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${WB_SHADOW_VS_SPV}" "$<TARGET_FILE_DIR:renderer_demo>/shaders/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${WB_SHADOW_FS_SPV}" "$<TARGET_FILE_DIR:renderer_demo>/shaders/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${WB_LIT_VS_SPV}" "$<TARGET_FILE_DIR:renderer_demo>/shaders/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${WB_LIT_FS_SPV}" "$<TARGET_FILE_DIR:renderer_demo>/shaders/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${WB_FSQ_VS_SPV}" "$<TARGET_FILE_DIR:renderer_demo>/shaders/"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${WB_COMP_FS_SPV}" "$<TARGET_FILE_DIR:renderer_demo>/shaders/"
  )
  if(WB_SHADERS_INCLUDE_COMP)
    list(APPEND _WB_RD_POST COMMAND ${CMAKE_COMMAND} -E copy_if_different "${WB_COMP_SPV}" "$<TARGET_FILE_DIR:renderer_demo>/shaders/")
  endif()
  add_custom_command(TARGET renderer_demo POST_BUILD ${_WB_RD_POST} VERBATIM)
endif()

enable_testing()
if(TARGET renderer_demo)
  add_test(NAME renderer_demo COMMAND renderer_demo)
  set_tests_properties(renderer_demo PROPERTIES ENVIRONMENT "WEAVEBOUND_SMOKE_FRAMES=0")
endif()
add_test(NAME engine_smoke COMMAND game_engine_smoke)
set_tests_properties(engine_smoke PROPERTIES
  ENVIRONMENT "WEAVEBOUND_SMOKE_FRAMES=0")

add_executable(weavebound_header_smoke engine/tests/header_smoke.cpp)
target_link_libraries(weavebound_header_smoke PRIVATE game_engine_core)
add_test(NAME weavebound_save_game_v0_smoke COMMAND weavebound_save_game_v0_smoke)
add_test(NAME weavebound_game_prototype COMMAND weavebound_game_prototype)
add_test(NAME weavebound_game_prototype_gameover COMMAND weavebound_game_prototype --ci-gameover)
add_test(NAME weavebound_game_prototype_ability_smoke COMMAND weavebound_game_prototype --ability-smoke)
add_test(NAME weavebound_game_prototype_settings_smoke COMMAND weavebound_game_prototype --settings-smoke)
add_test(NAME weavebound_game_prototype_m1_flow_smoke COMMAND weavebound_game_prototype --m1-flow-smoke)
add_test(NAME weavebound_game_prototype_economy_smoke COMMAND weavebound_game_prototype --economy-smoke)

add_test(NAME weavebound_header_smoke COMMAND weavebound_header_smoke)

add_executable(weavebound_scene_pack_smoke engine/tests/scene_pack_smoke.cpp)
target_link_libraries(weavebound_scene_pack_smoke PRIVATE weavebound_ecs weavebound_asset weavebound_scene)
add_test(NAME weavebound_scene_pack_smoke COMMAND weavebound_scene_pack_smoke)

add_executable(weavebound_asset_pak_smoke engine/tests/asset_pak_smoke.cpp)
target_link_libraries(weavebound_asset_pak_smoke PRIVATE weavebound_asset)
add_test(NAME weavebound_asset_pak_smoke COMMAND weavebound_asset_pak_smoke)

if(WB_HAS_VULKAN)
  add_executable(weavebound_asset_upload_smoke engine/tests/asset_upload_smoke.cpp)
  target_link_libraries(weavebound_asset_upload_smoke PRIVATE game_engine_core)
  add_test(NAME weavebound_asset_upload_smoke COMMAND weavebound_asset_upload_smoke)
endif()

add_executable(weavebound_integration_smoke engine/tests/integration_smoke.cpp)
target_link_libraries(weavebound_integration_smoke PRIVATE weavebound_integrations weavebound_audio
                                                                                 weavebound_scripting)
add_test(NAME weavebound_integration_smoke COMMAND weavebound_integration_smoke)

add_executable(weavebound_physics_smoke engine/tests/physics_smoke.cpp)
target_link_libraries(weavebound_physics_smoke PRIVATE weavebound_engine weavebound_integrations)
add_test(NAME weavebound_physics_smoke COMMAND weavebound_physics_smoke)

if(WIN32)
  add_executable(weavebound_input_smoke engine/tests/input_smoke.cpp)
  target_link_libraries(weavebound_input_smoke PRIVATE weavebound_platform weavebound_imgui)
  add_test(NAME weavebound_input_smoke COMMAND weavebound_input_smoke)
endif()

add_executable(weavebound_observability_smoke engine/tests/observability_smoke.cpp)
target_link_libraries(weavebound_observability_smoke PRIVATE weavebound_observability)
add_test(NAME weavebound_observability_smoke COMMAND weavebound_observability_smoke)

add_executable(weavebound_audio_smoke engine/tests/audio_smoke.cpp)
target_link_libraries(weavebound_audio_smoke PRIVATE weavebound_audio)
add_test(NAME weavebound_audio_smoke COMMAND weavebound_audio_smoke)

add_executable(weavebound_lua_smoke engine/tests/lua_smoke.cpp)
target_link_libraries(weavebound_lua_smoke PRIVATE weavebound_scripting)
add_test(NAME weavebound_lua_smoke COMMAND weavebound_lua_smoke)

if(WB_HAS_VULKAN)
  add_executable(weavebound_rhi_smoke apps/rhi_smoke/main.cpp)
  target_link_libraries(weavebound_rhi_smoke PRIVATE game_engine_core)
  add_test(NAME weavebound_rhi_smoke COMMAND weavebound_rhi_smoke)
endif()
