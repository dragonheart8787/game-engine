#pragma once

#include <cstdint>
#include <string_view>

namespace weavebound::asset {

/** glTF import / cook / runtime bundle 管線占位（規格 §1.6、§2.1 assetc）。 */
enum class CookStage : std::uint8_t { Import, Cook, BuildBundle };

struct ICookPipeline {
    virtual ~ICookPipeline() = default;
    virtual bool run_stage(CookStage stage, std::string_view project_root) = 0;
};

}  // namespace weavebound::asset
