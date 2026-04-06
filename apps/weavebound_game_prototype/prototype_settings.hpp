#pragma once

#include <string>

namespace weavebound::game_prototype {

/** 與 docs/game/SAVE_CONTRACT_V1.md 之 prototype_settings_v1 對齊。 */
struct PrototypeSettings {
  float master_volume{1.f};
  bool sound_enabled{true};
  bool tutorial_dismissed{false};
};

bool load_prototype_settings(PrototypeSettings& out, std::string& err, const char* argv0);

bool save_prototype_settings(const PrototypeSettings& in, std::string& err, const char* argv0);

}  // namespace weavebound::game_prototype
