#include "ability_spec_loader.hpp"

#include <cctype>
#include <cmath>
#include <fstream>
#include <sstream>

namespace weavebound::game_prototype {

namespace {

bool read_string_after_key(const std::string& s, const char* key, std::string& out) {
  const std::string q = std::string("\"") + key + "\"";
  size_t i = s.find(q);
  if (i == std::string::npos) {
    return false;
  }
  i = s.find(':', i);
  if (i == std::string::npos) {
    return false;
  }
  i = s.find('"', i);
  if (i == std::string::npos) {
    return false;
  }
  const size_t j = s.find('"', i + 1);
  if (j == std::string::npos) {
    return false;
  }
  out = s.substr(i + 1, j - i - 1);
  return true;
}

bool read_float_after_key(const std::string& s, const char* key, float& out) {
  const std::string q = std::string("\"") + key + "\"";
  size_t i = s.find(q);
  if (i == std::string::npos) {
    return false;
  }
  i = s.find(':', i);
  if (i == std::string::npos) {
    return false;
  }
  ++i;
  while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) {
    ++i;
  }
  try {
    size_t n = 0;
    out = std::stof(s.substr(i), &n);
    return true;
  } catch (...) {
    return false;
  }
}

bool read_int_after_key(const std::string& s, const char* key, int& out) {
  const std::string q = std::string("\"") + key + "\"";
  size_t i = s.find(q);
  if (i == std::string::npos) {
    return false;
  }
  i = s.find(':', i);
  if (i == std::string::npos) {
    return false;
  }
  ++i;
  while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) {
    ++i;
  }
  try {
    size_t n = 0;
    out = std::stoi(s.substr(i), &n);
    return true;
  } catch (...) {
    return false;
  }
}

}  // namespace

bool load_ability_slice_v0_from_string(const std::string& json_text, AbilitySliceRuntime& out,
                                       std::string& err) {
  err.clear();
  const size_t meta_pos = json_text.find("\"meta\"");
  const std::string meta_sub =
      (meta_pos != std::string::npos) ? json_text.substr(meta_pos) : json_text;

  std::string ver;
  if (!read_string_after_key(meta_sub, "schema_version", ver)) {
    err = "missing schema_version";
    return false;
  }
  if (ver != kAbilitySliceSchemaVersion) {
    err = "schema_version mismatch";
    return false;
  }
  out.schema_version = ver;
  (void)read_string_after_key(meta_sub, "id", out.id);

  float ftmp = 0.f;
  int itmp = 0;
  if (read_float_after_key(meta_sub, "focus_max", ftmp)) {
    out.focus_max = ftmp;
  }

  const size_t dash_pos = json_text.find("\"dash\"");
  const std::string dash_sub =
      (dash_pos != std::string::npos) ? json_text.substr(dash_pos) : json_text;
  if (read_float_after_key(dash_sub, "cooldown_s", ftmp)) {
    out.dash_cooldown_s = ftmp;
  }
  if (read_float_after_key(dash_sub, "duration_s", ftmp)) {
    out.dash_duration_s = ftmp;
  }
  if (read_float_after_key(dash_sub, "speed_mult", ftmp)) {
    out.dash_speed_mult = ftmp;
  }
  if (read_float_after_key(dash_sub, "iframes_s", ftmp)) {
    out.dash_iframes_s = ftmp;
  }

  const size_t primary_pos = json_text.find("\"primary\"");
  std::string primary_sub;
  if (primary_pos != std::string::npos) {
    primary_sub = json_text.substr(primary_pos);
  }
  if (!primary_sub.empty()) {
    if (read_float_after_key(primary_sub, "range", ftmp)) {
      out.primary_range = ftmp;
    }
    if (read_float_after_key(primary_sub, "half_angle_deg", ftmp)) {
      out.primary_half_angle_deg = ftmp;
    }
    if (read_float_after_key(primary_sub, "windup_s", ftmp)) {
      out.primary_windup_s = ftmp;
    }
    if (read_float_after_key(primary_sub, "active_s", ftmp)) {
      out.primary_active_s = ftmp;
    }
    if (read_float_after_key(primary_sub, "damage", ftmp)) {
      out.primary_damage = ftmp;
    }
  }
  const size_t cost_pos = json_text.find("\"cost\"");
  if (cost_pos != std::string::npos) {
    const std::string cost_sub = json_text.substr(cost_pos);
    if (read_int_after_key(cost_sub, "amount", itmp)) {
      out.cost_amount = itmp;
    }
  }
  return true;
}

bool load_ability_slice_v0_from_file(const std::string& path, AbilitySliceRuntime& out, std::string& err) {
  std::ifstream f(path, std::ios::binary);
  if (!f) {
    err = "open failed: " + path;
    return false;
  }
  std::ostringstream ss;
  ss << f.rdbuf();
  return load_ability_slice_v0_from_string(ss.str(), out, err);
}

}  // namespace weavebound::game_prototype
