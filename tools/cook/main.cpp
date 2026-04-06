// Cook：.wbmesh（可選自 .glb 經 tinygltf）；可選 1x1 白貼圖 .wbtexture。
#include "cook_gltf.hpp"

#include "weavebound/asset/format.hpp"

#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

static std::uint32_t rough_vertex_hint_from_glb(const char* path) {
  std::ifstream f(path, std::ios::binary);
  if (!f) {
    return 3;
  }
  char magic[4]{};
  f.read(magic, 4);
  if (magic[0] != 'g' || magic[1] != 'l' || magic[2] != 'T' || magic[3] != 'F') {
    return 3;
  }
  std::uint32_t version = 0;
  f.read(reinterpret_cast<char*>(&version), 4);
  std::uint32_t length = 0;
  f.read(reinterpret_cast<char*>(&length), 4);
  (void)length;
  std::uint32_t chunk_len = 0;
  f.read(reinterpret_cast<char*>(&chunk_len), 4);
  char chunk_type[4]{};
  f.read(chunk_type, 4);
  if (chunk_type[0] != 'J' || chunk_type[1] != 'S' || chunk_type[2] != 'O' || chunk_type[3] != 'N') {
    return 3;
  }
  const std::size_t cap = chunk_len < 2'000'000 ? static_cast<std::size_t>(chunk_len) : 2'000'000;
  std::string json(cap, '\0');
  f.read(json.data(), static_cast<std::streamsize>(cap));
  const std::string key = "\"count\":";
  const auto pos = json.find(key);
  if (pos == std::string::npos) {
    return 3;
  }
  std::size_t i = pos + key.size();
  while (i < json.size() && (json[i] == ' ' || json[i] == '\t')) {
    ++i;
  }
  std::uint32_t n = 0;
  while (i < json.size() && json[i] >= '0' && json[i] <= '9') {
    n = n * 10u + static_cast<std::uint32_t>(json[i] - '0');
    ++i;
    if (n > 1'000'000u) {
      break;
    }
  }
  return n > 0 ? n : 3u;
}

static bool ends_with(const char* s, const char* suf) {
  if (!s || !suf) {
    return false;
  }
  const std::size_t ns = std::strlen(s);
  const std::size_t nu = std::strlen(suf);
  return ns >= nu && std::strcmp(s + ns - nu, suf) == 0;
}

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "usage: weavebound_cook <output.wbmesh>\n"
                 "       weavebound_cook <input.glb> <output.wbmesh>\n"
                 "       weavebound_cook ... --tex-out <white.wbtexture>\n";
    return 1;
  }

  const char* tex_out = nullptr;
  std::vector<const char*> pos;
  for (int i = 1; i < argc; ++i) {
    if (std::strcmp(argv[i], "--tex-out") == 0) {
      if (i + 1 < argc) {
        tex_out = argv[i + 1];
      }
      ++i;
      continue;
    }
    pos.push_back(argv[i]);
  }

  if (pos.size() >= 2u && ends_with(pos[0], ".glb")) {
    std::string err;
    if (cook_glb_to_wbmesh(pos[0], pos[1], err)) {
      std::cout << "cooked glb -> " << pos[1] << '\n';
    } else {
      std::cerr << "glb cook failed: " << err << " (fallback stub)\n";
      weavebound::asset::WbmeshHeader h{};
      h.vertex_count = rough_vertex_hint_from_glb(pos[0]);
      h.index_count = h.vertex_count;
      h.vertex_stride = 32;
      std::ofstream out(pos[1], std::ios::binary);
      if (!out) {
        return 2;
      }
      out.write(reinterpret_cast<const char*>(&h), sizeof(h));
    }
  } else if (pos.size() >= 1u) {
    weavebound::asset::WbmeshHeader h{};
    h.vertex_count = 3;
    h.index_count = 3;
    h.vertex_stride = 32;
    const char* out_path = pos[0];
    if (pos.size() >= 2u) {
      out_path = pos[1];
      h.vertex_count = rough_vertex_hint_from_glb(pos[0]);
      h.index_count = h.vertex_count;
      std::cout << "glb hint vertex_count=" << h.vertex_count << '\n';
    }
    std::ofstream out(out_path, std::ios::binary);
    if (!out) {
      return 2;
    }
    out.write(reinterpret_cast<const char*>(&h), sizeof(h));
    std::cout << "wrote wbmesh " << out_path << '\n';
  } else {
    return 1;
  }

  if (tex_out) {
    if (write_white_texture_rgba8(tex_out)) {
      std::cout << "wrote wbtexture " << tex_out << '\n';
    } else {
      return 3;
    }
  }

  return 0;
}
