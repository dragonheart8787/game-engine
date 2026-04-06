#pragma once

#include <cstdint>

namespace weavebound::asset {

constexpr std::uint32_t kWbmeshMagic = 0x57424d48u;  // 'WBMH'
constexpr std::uint32_t kWbtextureMagic = 0x57425458u;  // 'WBTX'
constexpr std::uint32_t kWbpakMagic = 0x5742504bu;    // 'WBPK'

/** 二進位 mesh 檔頭（vertex/index blob 隨後；與 glTF cook 對齊時擴充）。 */
struct WbmeshHeader {
  std::uint32_t magic{kWbmeshMagic};
  std::uint32_t version{1};
  std::uint32_t vertex_count{0};
  std::uint32_t index_count{0};
  std::uint32_t vertex_stride{0};
  std::uint32_t flags{0};
};

/** 二進位紋理檔頭（像素資料隨後；M3 預設 RGBA8）。 */
struct WbtextureHeader {
  std::uint32_t magic{kWbtextureMagic};
  std::uint32_t version{1};
  std::uint32_t width_px{0};
  std::uint32_t height_px{0};
  /** 0 = rgba8_unorm；其餘保留。 */
  std::uint32_t format{0};
  std::uint32_t mip_count{1};
  std::uint32_t reserved{0};
};

/** Bundle manifest 單筆（hash 用於增量 cook）。 */
struct ManifestEntry {
  char logical_name[128]{};
  std::uint64_t content_hash{0};
  std::uint64_t byte_offset{0};
  std::uint64_t byte_size{0};
};

}  // namespace weavebound::asset
