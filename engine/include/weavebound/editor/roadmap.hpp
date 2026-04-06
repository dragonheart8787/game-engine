#pragma once

#include <cstdint>

namespace weavebound::editor {

/** 工具鏈階段：v0 headless 場景、v1 簡易編輯器（規格 2.2）。 */
enum class EditorPhase : std::uint8_t { V0_HeadlessScene, V1_SimpleViewport, V2_FullAuthoring };

}  // namespace weavebound::editor
