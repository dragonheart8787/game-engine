#pragma once

#include "ability_spec.hpp"

#include <string>

namespace weavebound::game_prototype {

bool load_ability_slice_v0_from_string(const std::string& json_text, AbilitySliceRuntime& out,
                                       std::string& err);

bool load_ability_slice_v0_from_file(const std::string& path, AbilitySliceRuntime& out, std::string& err);

}  // namespace weavebound::game_prototype
