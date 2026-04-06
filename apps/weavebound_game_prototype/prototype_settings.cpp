#include "prototype_settings.hpp"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace weavebound::game_prototype {

namespace {

constexpr const char* kFileName = "weavebound_prototype_settings.json";

bool read_float_after(const std::string& s, const char* key, float& out) {
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

bool read_bool_after(const std::string& s, const char* key, bool& out) {
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
  if (i + 4 <= s.size() && s.substr(i, 4) == "true") {
    out = true;
    return true;
  }
  if (i + 5 <= s.size() && s.substr(i, 5) == "false") {
    out = false;
    return true;
  }
  return false;
}

bool try_read_file(const std::string& path, std::string& out_text) {
  std::ifstream f(path, std::ios::binary);
  if (!f) {
    return false;
  }
  std::ostringstream ss;
  ss << f.rdbuf();
  out_text = ss.str();
  return true;
}

#if defined(_WIN32)
std::wstring exe_dir_wide() {
  wchar_t wb[2048];
  if (GetModuleFileNameW(nullptr, wb, 2048) == 0) {
    return L"";
  }
  std::wstring w(wb);
  const size_t sl = w.find_last_of(L"\\/");
  if (sl != std::wstring::npos) {
    w.resize(sl + 1);
  } else {
    w.clear();
  }
  return w;
}
#endif

}  // namespace

bool load_prototype_settings(PrototypeSettings& out, std::string& err, const char* argv0) {
  err.clear();
  out = PrototypeSettings{};
  std::string raw;
  if (!try_read_file(kFileName, raw)) {
    if (argv0) {
      const std::filesystem::path base(argv0);
      try_read_file((base.parent_path() / kFileName).string(), raw);
    }
  }
#if defined(_WIN32)
  if (raw.empty()) {
    const std::wstring w = exe_dir_wide() + std::wstring(kFileName, kFileName + strlen(kFileName));
    std::ifstream wf(w, std::ios::binary);
    if (wf) {
      std::ostringstream ss;
      ss << wf.rdbuf();
      raw = ss.str();
    }
  }
#endif
  if (raw.empty()) {
    return true;
  }
  float vol = 1.f;
  if (read_float_after(raw, "master_volume", vol)) {
    out.master_volume = std::clamp(vol, 0.f, 1.f);
  }
  bool se = true;
  if (read_bool_after(raw, "sound_enabled", se)) {
    out.sound_enabled = se;
  }
  bool td = false;
  if (read_bool_after(raw, "tutorial_dismissed", td)) {
    out.tutorial_dismissed = td;
  }
  return true;
}

bool save_prototype_settings(const PrototypeSettings& in, std::string& err, const char* argv0) {
  err.clear();
  std::ostringstream oss;
  oss << "{\n  \"schema_version\": \"prototype_settings_v1\",\n";
  oss << "  \"master_volume\": " << static_cast<double>(in.master_volume) << ",\n";
  oss << "  \"sound_enabled\": " << (in.sound_enabled ? "true" : "false") << ",\n";
  oss << "  \"tutorial_dismissed\": " << (in.tutorial_dismissed ? "true" : "false") << "\n}\n";
  const std::string content = oss.str();

  std::string path = kFileName;
  if (argv0) {
    const std::filesystem::path base(argv0);
    path = (base.parent_path() / kFileName).string();
  }
  {
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (f) {
      f.write(content.data(), static_cast<std::streamsize>(content.size()));
      return static_cast<bool>(f);
    }
  }
#if defined(_WIN32)
  {
    const std::wstring w = exe_dir_wide() + std::wstring(kFileName, kFileName + strlen(kFileName));
    std::ofstream wf(w, std::ios::binary | std::ios::trunc);
    if (wf) {
      wf.write(content.data(), static_cast<std::streamsize>(content.size()));
      return static_cast<bool>(wf);
    }
  }
#endif
  err = "failed to write settings";
  return false;
}

}  // namespace weavebound::game_prototype
