#include "weavebound/asset/format.hpp"
#include "weavebound/asset/wbpak.hpp"
#include "weavebound/platform/vfs.hpp"

#include <cstring>
#include <vector>

int main() {
  using namespace weavebound;

  asset::WbmeshHeader h{};
  h.vertex_count = 7;
  h.index_count = 12;
  h.vertex_stride = 32;

  std::vector<std::uint8_t> mesh(sizeof(h));
  std::memcpy(mesh.data(), &h, sizeof(h));

  asset::WbpakFileEntry e{};
  e.logical_name = "meshes/test.wbmesh";
  e.bytes = std::move(mesh);

  std::vector<std::uint8_t> pak = asset::build_wbpak({e});
  if (pak.empty()) {
    return 2;
  }

  std::unique_ptr<platform::IVfs> vfs = platform::create_memory_pak_vfs(std::move(pak));
  if (!vfs || !vfs->exists("meshes/test.wbmesh")) {
    return 3;
  }

  bool ok = false;
  vfs->read_all("meshes/test.wbmesh", [&](const std::uint8_t* p, std::size_t n) {
    if (n < sizeof(asset::WbmeshHeader)) {
      return;
    }
    const auto* rh = reinterpret_cast<const asset::WbmeshHeader*>(p);
    ok = rh->magic == asset::kWbmeshMagic && rh->vertex_count == 7u;
  });
  return ok ? 0 : 4;
}
