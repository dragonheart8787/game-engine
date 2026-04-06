#include "weavebound/platform/vfs.hpp"

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace weavebound::platform {

namespace {

class DiskVfs final : public IVfs {
 public:
  explicit DiskVfs(std::filesystem::path root) : root_(std::move(root)) {}

  bool exists(std::string_view logical_path) const override {
    return std::filesystem::is_regular_file(resolve(logical_path));
  }

  bool read_all(std::string_view logical_path,
                std::function<void(const std::uint8_t*, std::size_t)> sink) const override {
    const auto p = resolve(logical_path);
    std::ifstream f(p, std::ios::binary | std::ios::ate);
    if (!f) {
      return false;
    }
    const auto sz = static_cast<std::size_t>(f.tellg());
    f.seekg(0);
    std::vector<std::uint8_t> buf(sz);
    if (sz > 0) {
      f.read(reinterpret_cast<char*>(buf.data()), static_cast<std::streamsize>(sz));
    }
    sink(buf.data(), buf.size());
    return true;
  }

 private:
  std::filesystem::path resolve(std::string_view logical_path) const {
    std::string s(logical_path);
    return root_ / s;
  }

  std::filesystem::path root_;
};

}  // namespace

std::unique_ptr<IVfs> create_disk_vfs(std::filesystem::path root_dir) {
  return std::make_unique<DiskVfs>(std::move(root_dir));
}

}  // namespace weavebound::platform
