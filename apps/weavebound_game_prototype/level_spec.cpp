#include "level_spec.hpp"

#include <fstream>
#include <sstream>

namespace weavebound::game_prototype {

namespace {

bool read_string_value_after_key(const std::string& s, size_t search_from, const char* key, std::string& out,
                                 size_t& end_pos) {
  const std::string q = std::string("\"") + key + "\"";
  const size_t i = s.find(q, search_from);
  if (i == std::string::npos) {
    return false;
  }
  size_t j = s.find(':', i);
  if (j == std::string::npos) {
    return false;
  }
  j = s.find('"', j);
  if (j == std::string::npos) {
    return false;
  }
  const size_t k = s.find('"', j + 1);
  if (k == std::string::npos) {
    return false;
  }
  out = s.substr(j + 1, k - j - 1);
  end_pos = k + 1;
  return true;
}

}  // namespace

bool load_level_m1_from_string(const std::string& json_text, LevelM1Spec& out, std::string& err) {
  err.clear();
  out = LevelM1Spec{};
  size_t z = 0;
  if (!read_string_value_after_key(json_text, 0, "schema_version", out.schema_version, z)) {
    err = "missing schema_version";
    return false;
  }
  if (out.schema_version != "level_m1_v1") {
    err = "schema_version mismatch";
    return false;
  }

  size_t cursor = json_text.find("\"phases\"");
  if (cursor == std::string::npos) {
    err = "missing phases";
    return false;
  }
  cursor = json_text.find('[', cursor);
  if (cursor == std::string::npos) {
    err = "phases not array";
    return false;
  }

  while (out.phases.size() < 16u) {
    LevelM1Phase ph{};
    size_t next = 0;
    if (!read_string_value_after_key(json_text, cursor, "objective", ph.objective, next)) {
      break;
    }
    if (!read_string_value_after_key(json_text, next, "hint", ph.hint, next)) {
      err = "phase missing hint";
      return false;
    }
    out.phases.push_back(std::move(ph));
    cursor = next;
  }

  if (out.phases.size() < 3u) {
    err = "need at least 3 phases";
    return false;
  }
  return true;
}

bool load_level_m1_from_file(const std::string& path, LevelM1Spec& out, std::string& err) {
  std::ifstream f(path, std::ios::binary);
  if (!f) {
    err = "open failed: " + path;
    return false;
  }
  std::ostringstream ss;
  ss << f.rdbuf();
  return load_level_m1_from_string(ss.str(), out, err);
}

}  // namespace weavebound::game_prototype
