#pragma once

#include <string>
#include <vector>

namespace weavebound::game_prototype {

/** 與 data/level_m1.json 對齊（M1 單關三階節奏）。 */
struct LevelM1Phase {
  std::string objective;
  std::string hint;
};

struct LevelM1Spec {
  std::string schema_version;
  std::vector<LevelM1Phase> phases;
};

bool load_level_m1_from_string(const std::string& json_text, LevelM1Spec& out, std::string& err);

bool load_level_m1_from_file(const std::string& path, LevelM1Spec& out, std::string& err);

}  // namespace weavebound::game_prototype
