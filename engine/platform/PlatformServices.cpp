#include "engine/platform/PlatformServices.h"

#include <chrono>
#include <fstream>
#include <sstream>

#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace engine::platform {

bool VirtualFileSystem::mount(const std::string& mountPoint, const std::string& sourcePath) {
  if (mountPoint.empty() || sourcePath.empty()) {
    return false;
  }
  mounts_[mountPoint] = sourcePath;
  return true;
}

std::optional<std::string> VirtualFileSystem::resolve(const std::string& virtualPath) const {
  std::size_t bestLen = 0;
  std::optional<std::string> resolved;
  for (const auto& [mount, source] : mounts_) {
    if (virtualPath.rfind(mount, 0) == 0 && mount.size() >= bestLen) {
      bestLen = mount.size();
      resolved = source + virtualPath.substr(mount.size());
    }
  }
  return resolved;
}

std::optional<std::string> VirtualFileSystem::readTextFile(const std::string& virtualPath) const {
  auto resolved = resolve(virtualPath);
  if (!resolved.has_value()) {
    return std::nullopt;
  }

  std::ifstream in(*resolved, std::ios::binary);
  if (!in.is_open()) {
    return std::nullopt;
  }

  std::ostringstream stream;
  stream << in.rdbuf();
  return stream.str();
}

bool VirtualFileSystem::exists(const std::string& virtualPath) const {
  auto resolved = resolve(virtualPath);
  if (!resolved.has_value()) {
    return false;
  }
  std::ifstream in(*resolved, std::ios::binary);
  return in.good();
}

std::uint64_t MonotonicFixedTimeSource::monotonicNowNs() const {
  const auto now = std::chrono::steady_clock::now().time_since_epoch();
  return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(now).count());
}

std::uint64_t MonotonicFixedTimeSource::fixedTick() const {
  return fixedTick_.load(std::memory_order_relaxed);
}

void MonotonicFixedTimeSource::advanceFixedTick() {
  fixedTick_.fetch_add(1, std::memory_order_relaxed);
}

WorkerPool::WorkerPool(std::size_t workerCount) {
  if (workerCount == 0) {
    workerCount = 1;
  }
  workers_.reserve(workerCount);
  for (std::size_t i = 0; i < workerCount; ++i) {
    workers_.emplace_back([this]() { workerMain(); });
  }
}

WorkerPool::~WorkerPool() {
  {
    std::lock_guard<std::mutex> lock(mutex_);
    stopping_ = true;
  }
  cv_.notify_all();
  for (std::thread& worker : workers_) {
    if (worker.joinable()) {
      worker.join();
    }
  }
}

std::future<void> WorkerPool::enqueue(std::function<void()> task) {
  std::packaged_task<void()> packaged(std::move(task));
  auto future = packaged.get_future();
  {
    std::lock_guard<std::mutex> lock(mutex_);
    tasks_.push(std::move(packaged));
  }
  cv_.notify_one();
  return future;
}

void WorkerPool::workerMain() {
  while (true) {
    std::packaged_task<void()> task;
    {
      std::unique_lock<std::mutex> lock(mutex_);
      cv_.wait(lock, [this]() { return stopping_ || !tasks_.empty(); });
      if (stopping_ && tasks_.empty()) {
        return;
      }
      task = std::move(tasks_.front());
      tasks_.pop();
    }
    task();
  }
}

DynamicPluginLoader::~DynamicPluginLoader() {
  unload();
}

bool DynamicPluginLoader::load(const std::string& path, PluginLoadMode mode) {
  unload();
#if defined(_WIN32)
  const DWORD flags = mode == PluginLoadMode::Now ? 0 : DONT_RESOLVE_DLL_REFERENCES;
  handle_ = static_cast<void*>(LoadLibraryExA(path.c_str(), nullptr, flags));
#else
  const int flags = mode == PluginLoadMode::Now ? RTLD_NOW : RTLD_LAZY;
  handle_ = dlopen(path.c_str(), flags);
#endif
  return handle_ != nullptr;
}

void* DynamicPluginLoader::symbol(const std::string& name) {
  if (handle_ == nullptr) {
    return nullptr;
  }
#if defined(_WIN32)
  return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(handle_), name.c_str()));
#else
  return dlsym(handle_, name.c_str());
#endif
}

void DynamicPluginLoader::unload() {
  if (handle_ == nullptr) {
    return;
  }
#if defined(_WIN32)
  FreeLibrary(static_cast<HMODULE>(handle_));
#else
  dlclose(handle_);
#endif
  handle_ = nullptr;
}

void CrashReporter::install() {
  // Placeholder for platform specific unhandled-exception/signal hooks.
}

void CrashReporter::capture(const CrashContext& context) {
  std::lock_guard<std::mutex> lock(mutex_);
  lastCrash_ = context;
}

std::optional<CrashContext> CrashReporter::lastCrash() const {
  std::lock_guard<std::mutex> lock(mutex_);
  return lastCrash_;
}

}  // namespace engine::platform
