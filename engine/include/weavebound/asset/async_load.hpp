#pragma once

#include <cstdint>
#include <fstream>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "weavebound/platform/job_system.hpp"

namespace weavebound::asset {

/** 非同步（工作佇列）讀檔；M3 最小版：在 job 內同步讀取整檔。 */
inline void enqueue_read_file_async(platform::IJobSystem& jobs, const char* path,
                                    std::function<void(std::vector<std::uint8_t>)> on_done) {
  jobs.submit([path = std::string(path ? path : ""), on_done = std::move(on_done)]() mutable {
    std::vector<std::uint8_t> buf;
    if (!path.empty()) {
      std::ifstream f(path, std::ios::binary | std::ios::ate);
      if (f) {
        const auto sz = static_cast<std::size_t>(f.tellg());
        buf.resize(sz);
        f.seekg(0);
        f.read(reinterpret_cast<char*>(buf.data()), static_cast<std::streamsize>(sz));
      }
    }
    on_done(std::move(buf));
  });
}

}  // namespace weavebound::asset
