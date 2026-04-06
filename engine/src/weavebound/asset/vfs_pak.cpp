#include "weavebound/platform/vfs.hpp"

#include "weavebound/asset/wbpak.hpp"

#include <utility>
#include <vector>

namespace weavebound::platform {

namespace {

class PakVfs final : public IVfs {
 public:
  explicit PakVfs(std::vector<std::uint8_t> bytes) : pak_(std::move(bytes)) {}

  bool exists(std::string_view logical_path) const override {
    std::size_t off = 0;
    std::size_t sz = 0;
    return asset::wbpak_find(pak_.data(), pak_.size(), logical_path, off, sz);
  }

  bool read_all(std::string_view logical_path,
                std::function<void(const std::uint8_t*, std::size_t)> sink) const override {
    std::size_t off = 0;
    std::size_t sz = 0;
    if (!asset::wbpak_find(pak_.data(), pak_.size(), logical_path, off, sz)) {
      return false;
    }
    sink(pak_.data() + off, sz);
    return true;
  }

 private:
  std::vector<std::uint8_t> pak_;
};

}  // namespace

std::unique_ptr<IVfs> create_memory_pak_vfs(std::vector<std::uint8_t> pak_bytes) {
  return std::make_unique<PakVfs>(std::move(pak_bytes));
}

}  // namespace weavebound::platform
