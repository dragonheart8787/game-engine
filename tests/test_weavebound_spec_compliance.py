"""
WeaveBound 規格「存在性 + 可執行契約」測試。

說明：完整功能（Metal 後端、PBR、Jolt 等）無法在單測中一次驗證；
此模組斷言：文件承諾、公開標頭/原始碼占位、CLI 子命令與場景驗證可跑通。
"""

from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path

import pytest

REPO_ROOT = Path(__file__).resolve().parents[1]
SPEC_PATH = REPO_ROOT / "docs" / "WEAVEBOUND_SPEC.md"
INCLUDE = REPO_ROOT / "engine" / "include" / "weavebound"


def _must_exist(rel: str) -> Path:
    p = REPO_ROOT / rel
    assert p.is_file(), f"missing artifact: {rel}"
    return p


@pytest.mark.parametrize(
    "rel",
    [
        "engine/include/weavebound/weavebound.hpp",
        "engine/include/weavebound/platform/window.hpp",
        "engine/include/weavebound/platform/input.hpp",
        "engine/include/weavebound/platform/vfs.hpp",
        "engine/include/weavebound/platform/frame_pacing.hpp",
        "engine/include/weavebound/platform/clock.hpp",
        "engine/include/weavebound/platform/job_system.hpp",
        "engine/include/weavebound/platform/system_info.hpp",
        "engine/include/weavebound/platform/crash_handler.hpp",
        "engine/include/weavebound/platform/delta_smoothing.hpp",
        "engine/include/weavebound/rhi/device.hpp",
        "engine/include/weavebound/rhi/lit_demo_frame.hpp",
        "engine/include/weavebound/rhi/types.hpp",
        "engine/include/weavebound/rhi/resources.hpp",
        "engine/include/weavebound/render_graph/graph.hpp",
        "engine/include/weavebound/render_graph/ir.hpp",
        "engine/include/weavebound/render_graph/builder.hpp",
        "engine/include/weavebound/render_graph/compiled.hpp",
        "engine/src/weavebound/render_graph/compiler.cpp",
        "engine/include/weavebound/rhi/pipeline.hpp",
        "engine/include/weavebound/rhi/command.hpp",
        "engine/include/weavebound/rhi/binding.hpp",
        "engine/include/weavebound/renderer/forward_plus_phase1.hpp",
        "engine/include/weavebound/ecs/scene_types.hpp",
        "engine/include/weavebound/ecs/registry.hpp",
        "engine/src/weavebound/ecs/registry.cpp",
        "engine/include/weavebound/engine/application.hpp",
        "engine/src/weavebound/engine/application.cpp",
        "engine/include/weavebound/asset/pipeline.hpp",
        "engine/include/weavebound/asset/async_load_rhi.hpp",
        "engine/include/weavebound/physics/world.hpp",
        "engine/include/weavebound/audio/mixer.hpp",
        "engine/include/weavebound/ui/immediate.hpp",
        "engine/include/weavebound/scripting/lua_host.hpp",
        "engine/include/weavebound/observability/profiler.hpp",
        "engine/include/weavebound/observability/logger.hpp",
        "engine/include/weavebound/net/driver.hpp",
        "engine/include/weavebound/net/replication.hpp",
        "engine/include/weavebound/runtime/save_system.hpp",
        "engine/include/weavebound/runtime/config_system.hpp",
        "engine/include/weavebound/editor/roadmap.hpp",
        "CMakeLists.txt",
        "apps/smoke/main.cpp",
        "engine/tests/header_smoke.cpp",
        "docs/ADR/0001-rhi-object-model.md",
        "docs/ADR/0002-render-graph-ir.md",
        "docs/ADR/0003-ecs-storage.md",
        "docs/ADR/0004-input-action-map.md",
        "docs/ADR/0005-jolt-coordinates-and-units.md",
        "engine/include/weavebound/observability/frame_meter.hpp",
        "engine/src/weavebound/observability/frame_meter.cpp",
        "engine/include/weavebound/physics/jolt_world.hpp",
        "engine/src/weavebound/physics/jolt_world.cpp",
        "engine/tests/input_smoke.cpp",
        "engine/tests/observability_smoke.cpp",
        "docs/game/TDD.md",
        "docs/game/GDD.md",
        "engine/include/weavebound/game/save_game_v0.hpp",
        "engine/src/weavebound/game/save_game_v0.cpp",
        "apps/weavebound_game_prototype/main.cpp",
        "apps/weavebound_game_prototype/game_flow.hpp",
        "apps/weavebound_game_prototype/game_flow.cpp",
    ],
)
def test_spec_artifact_files_exist(rel: str) -> None:
    _must_exist(rel)


def test_vulkan_backend_source_present() -> None:
    """RHI：Vulkan 實作檔存在（實際連結依 SDK / 平台）。"""
    vk = REPO_ROOT / "engine" / "src" / "weavebound" / "rhi" / "vulkan" / "device_vulkan.cpp"
    generic = REPO_ROOT / "engine" / "src" / "weavebound" / "rhi" / "device.cpp"
    assert vk.is_file()
    assert generic.is_file()


def test_rhi_backend_enum_documents_api_surface() -> None:
    text = (REPO_ROOT / "engine" / "include" / "weavebound" / "rhi" / "types.hpp").read_text(
        encoding="utf-8"
    )
    assert "Vulkan" in text and "Metal" in text and "D3d12" in text


def test_weavebound_spec_document_covers_product_positioning() -> None:
    assert SPEC_PATH.is_file()
    text = SPEC_PATH.read_text(encoding="utf-8")
    required = [
        "Forward+",
        "Render Graph",
        "RHI",
        "Vulkan",
        "Metal",
        "Win64",
        "glTF",
        "assetc",
        "scene_tool",
        "packager",
        "RenderGraphBuilder",
        "docs/ADR/",
    ]
    missing = [k for k in required if k not in text]
    assert not missing, f"WEAVEBOUND_SPEC.md missing keywords: {missing}"


def test_render_graph_pass_kinds_include_compute_and_copy() -> None:
    ir_text = (INCLUDE / "render_graph" / "ir.hpp").read_text(encoding="utf-8")
    assert "Compute" in ir_text and "Copy" in ir_text


def test_compiler_json_contains_pass_order_and_barriers() -> None:
    """編譯器產物字串層級契約（不依賴 CMake）。"""
    cpp = (REPO_ROOT / "engine" / "src" / "weavebound" / "render_graph" / "compiler.cpp").read_text(
        encoding="utf-8"
    )
    assert "pass_order" in cpp and r"\"schema\":1" in cpp and "color_attachment_to_srv" in cpp
    assert "depth_attachment_to_srv" in cpp and "vk_aspect_mask" in cpp


def test_vulkan_image_ops_sources_exist() -> None:
    assert (REPO_ROOT / "engine" / "src" / "weavebound" / "rhi" / "vulkan" / "image_ops.cpp").is_file()
    h = (REPO_ROOT / "engine" / "include" / "weavebound" / "rhi" / "vulkan" / "image_ops.hpp").read_text(
        encoding="utf-8"
    )
    assert "cmd_image_barrier" in h


def _run_cli(args: list[str]) -> subprocess.CompletedProcess[str]:
    src = str(REPO_ROOT / "src")
    env = os.environ.copy()
    prev = env.get("PYTHONPATH", "")
    env["PYTHONPATH"] = src if not prev else f"{src}{os.pathsep}{prev}"
    return subprocess.run(
        [sys.executable, "-m", "game_engine.cli", *args],
        cwd=str(REPO_ROOT),
        env=env,
        capture_output=True,
        text=True,
        check=False,
    )


def test_cli_default_report_runs() -> None:
    p = _run_cli(["--format", "json"])
    assert p.returncode == 0
    assert "runtime_core" in p.stdout


@pytest.mark.parametrize(
    "args, expect_ok",
    [
        (["assetc", "import"], True),
        (["assetc", "cook"], True),
        (["assetc", "build"], True),
        (["packager", "--platform", "linux", "--bundle-format", "appimage"], True),
        (["scene_tool", "bake-prefab"], True),
        (["scene_tool", "upgrade-scene"], True),
    ],
)
def test_cli_tool_stubs(args: list[str], expect_ok: bool) -> None:
    p = _run_cli(args)
    assert p.returncode == (0 if expect_ok else 1), (p.stdout, p.stderr)


def test_cli_scene_tool_validate_fixture() -> None:
    fixture = REPO_ROOT / "tests" / "fixtures" / "minimal_valid_scene.yaml"
    p = _run_cli(["scene_tool", "validate", "--path", str(fixture)])
    assert p.returncode == 0, (p.stdout, p.stderr)


def test_pytest_contracts_still_importable() -> None:
    from game_engine.architecture.platform import PLATFORM_PILLARS, MVP_MILESTONES

    assert PLATFORM_PILLARS
    assert MVP_MILESTONES
