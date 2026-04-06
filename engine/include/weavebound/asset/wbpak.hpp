#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "weavebound/asset/format.hpp"

namespace weavebound::asset {

/** 建立 .wbpak：檔頭 + manifest + 串接之 blob（M3 最小實作）。 */
struct WbpakFileEntry {
  std::string logical_name;
  std::vector<std::uint8_t> bytes;
};

struct WbpakHeader {
  std::uint32_t magic{kWbpakMagic};
  std::uint32_t version{1};
  std::uint32_t entry_count{0};
  std::uint32_t reserved{0};
};

/** 序列化至位元組；失敗回傳空 vector。 */
std::vector<std::uint8_t> build_wbpak(const std::vector<WbpakFileEntry>& entries);

/**
 * 在已載入之 .wbpak 位元組中查找邏輯名稱（不含路徑正規化）。
 * 成功時 `out_offset`/`out_size` 指向 blob 區間。
 */
bool wbpak_find(const std::uint8_t* pak_data, std::size_t pak_size, std::string_view logical_name,
                std::size_t& out_offset, std::size_t& out_size);

}  // namespace weavebound::asset
