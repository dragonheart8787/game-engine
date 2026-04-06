#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <string_view>
#include <vector>

namespace weavebound::platform {

/** 虛擬路徑掛載點（規格 §1.1 File I/O）；實作待接實體檔案與 pak。 */
class IVfs {
 public:
  virtual ~IVfs() = default;

  virtual bool exists(std::string_view logical_path) const = 0;
  virtual bool read_all(std::string_view logical_path,
                        std::function<void(const std::uint8_t*, std::size_t)> sink) const = 0;
};

std::unique_ptr<IVfs> create_disk_vfs(std::filesystem::path root_dir);

/** 將整包 .wbpak 載入記憶體並以邏輯檔名讀取條目（不寫回磁碟）。 */
std::unique_ptr<IVfs> create_memory_pak_vfs(std::vector<std::uint8_t> pak_bytes);

}  // namespace weavebound::platform
