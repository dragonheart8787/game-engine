// 將多個檔案打成 .wbpak（manifest + blob）。
#include "weavebound/asset/wbpak.hpp"

#include <fstream>
#include <iostream>
#include <string>
#include <vector>

static bool read_file(const std::string& path, std::vector<std::uint8_t>& out) {
  std::ifstream f(path, std::ios::binary | std::ios::ate);
  if (!f) {
    return false;
  }
  const auto sz = static_cast<std::size_t>(f.tellg());
  out.resize(sz);
  f.seekg(0);
  f.read(reinterpret_cast<char*>(out.data()), static_cast<std::streamsize>(sz));
  return true;
}

int main(int argc, char** argv) {
  if (argc < 4 || std::string(argv[1]) != "pack") {
    std::cerr << "usage: weavebound_wbpak pack <out.wbpak> logical=path [logical2=path2 ...]\n";
    return 1;
  }
  std::vector<weavebound::asset::WbpakFileEntry> entries;
  for (int i = 3; i < argc; ++i) {
    std::string spec = argv[i];
    const auto eq = spec.find('=');
    if (eq == std::string::npos) {
      std::cerr << "bad entry (use name=path): " << spec << '\n';
      return 2;
    }
    weavebound::asset::WbpakFileEntry e{};
    e.logical_name = spec.substr(0, eq);
    const std::string p = spec.substr(eq + 1);
    if (!read_file(p, e.bytes)) {
      std::cerr << "cannot read: " << p << '\n';
      return 3;
    }
    entries.push_back(std::move(e));
  }
  const std::vector<std::uint8_t> pak = weavebound::asset::build_wbpak(entries);
  if (pak.empty()) {
    return 4;
  }
  std::ofstream out(argv[2], std::ios::binary);
  if (!out) {
    return 5;
  }
  out.write(reinterpret_cast<const char*>(pak.data()), static_cast<std::streamsize>(pak.size()));
  std::cout << "wrote " << argv[2] << " bytes=" << pak.size() << " entries=" << entries.size() << '\n';
  return 0;
}
