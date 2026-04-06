#pragma once

#include "weavebound/asset/async_load.hpp"
#include "weavebound/asset/format.hpp"
#include "weavebound/rhi/device.hpp"
#include "weavebound/rhi/resources.hpp"

#include <cstdint>
#include <functional>
#include <utility>
#include <vector>

namespace weavebound::asset {

/**
 * 在工作執行緒解析 .wbmesh 位元組並上傳頂點 blob 至 `IDevice`（最小 E2E：cook → 記憶體 → GPU buffer）。
 */
inline void enqueue_wbmesh_vertex_buffer_upload(
    platform::IJobSystem& jobs, rhi::IDevice& dev, std::vector<std::uint8_t> file_bytes,
    std::function<void(bool ok, rhi::BufferHandle buf)> on_done) {
  jobs.submit([&, data = std::move(file_bytes), on_done = std::move(on_done)]() mutable {
    if (data.size() < sizeof(WbmeshHeader)) {
      on_done(false, {});
      return;
    }
    const auto* h = reinterpret_cast<const WbmeshHeader*>(data.data());
    if (h->magic != kWbmeshMagic || h->vertex_stride == 0 || h->vertex_count == 0) {
      on_done(false, {});
      return;
    }
    const std::size_t vtx_bytes = static_cast<std::size_t>(h->vertex_count) * static_cast<std::size_t>(h->vertex_stride);
    const std::size_t idx_el = (h->flags & 1u) != 0 ? 4u : 2u;
    const std::size_t idx_bytes = static_cast<std::size_t>(h->index_count) * idx_el;
    const std::size_t need = sizeof(WbmeshHeader) + vtx_bytes + idx_bytes;
    if (data.size() < need) {
      on_done(false, {});
      return;
    }
    rhi::BufferDesc bd{};
    bd.size_bytes = vtx_bytes;
    bd.usage = rhi::BufferUsage::Vertex | rhi::BufferUsage::TransferDst;
    rhi::BufferHandle bh{};
    if (!dev.create_buffer(bd, bh)) {
      on_done(false, {});
      return;
    }
    const void* vsrc = data.data() + sizeof(WbmeshHeader);
    if (!dev.upload_buffer(bh, vsrc, vtx_bytes)) {
      on_done(false, bh);
      return;
    }
    on_done(true, bh);
  });
}

}  // namespace weavebound::asset
