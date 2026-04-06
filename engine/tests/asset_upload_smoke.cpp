// 需 Vulkan：wbmesh 位元組 → job → create_buffer / upload_buffer
#if !defined(_WIN32)
int main() { return 0; }
#else

#include "weavebound/asset/async_load_rhi.hpp"
#include "weavebound/asset/format.hpp"
#include "weavebound/engine/application.hpp"
#include "weavebound/platform/job_system.hpp"

#include <atomic>
#include <cstring>
#include <vector>

int main() {
  using namespace weavebound;

  engine::ApplicationConfig cfg;
  cfg.request_vulkan = true;
  cfg.visible = false;
  engine::Application app;
  if (!app.startup(cfg)) {
    return 0;
  }
  rhi::IDevice* dev = app.device();
  if (!dev || !dev->is_valid()) {
    return 0;
  }

  asset::WbmeshHeader h{};
  h.vertex_count = 1;
  h.index_count = 3;
  h.vertex_stride = 32;

  std::vector<std::uint8_t> buf(sizeof(asset::WbmeshHeader) + 32u + 3u * 2u, 0);
  std::memcpy(buf.data(), &h, sizeof(h));

  auto jobs = platform::create_inline_job_system();
  std::atomic<bool> done{false};
  bool ok = false;
  asset::enqueue_wbmesh_vertex_buffer_upload(
      *jobs, *dev, std::move(buf), [&](bool o, rhi::BufferHandle /*bh*/) {
        ok = o;
        done.store(true);
      });
  jobs->wait_all();
  if (!done.load()) {
    return 2;
  }
  return ok ? 0 : 3;
}

#endif
