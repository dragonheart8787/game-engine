#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace engine::platform {

enum class PluginLoadMode {
  Lazy,
  Now
};

struct WindowDescriptor {
  std::string title = "Game Engine";
  std::uint32_t width = 1280;
  std::uint32_t height = 720;
};

struct SwapchainSurfaceDescriptor {
  void* nativeWindow = nullptr;
  std::string backend;
};

struct SurfaceHandle {
  std::uint64_t id = 0;
  bool valid() const { return id != 0; }
};

struct InputSample {
  std::string action;
  float x = 0.0f;
  float y = 0.0f;
  std::int64_t fingerId = -1;
};

struct CrashContext {
  std::string reason;
  std::uint64_t fixedTick = 0;
  std::string buildId;
};

class IFileSystem {
public:
  virtual ~IFileSystem() = default;
  virtual bool mount(const std::string& mountPoint, const std::string& sourcePath) = 0;
  virtual std::optional<std::string> readTextFile(const std::string& virtualPath) const = 0;
  virtual bool exists(const std::string& virtualPath) const = 0;
};

class VirtualFileSystem final : public IFileSystem {
public:
  bool mount(const std::string& mountPoint, const std::string& sourcePath) override;
  std::optional<std::string> readTextFile(const std::string& virtualPath) const override;
  bool exists(const std::string& virtualPath) const override;

private:
  std::optional<std::string> resolve(const std::string& virtualPath) const;

private:
  std::unordered_map<std::string, std::string> mounts_;
};

class ITimeSource {
public:
  virtual ~ITimeSource() = default;
  virtual std::uint64_t monotonicNowNs() const = 0;
  virtual std::uint64_t fixedTick() const = 0;
  virtual void advanceFixedTick() = 0;
};

class MonotonicFixedTimeSource final : public ITimeSource {
public:
  std::uint64_t monotonicNowNs() const override;
  std::uint64_t fixedTick() const override;
  void advanceFixedTick() override;

private:
  std::atomic<std::uint64_t> fixedTick_{0};
};

class IThreading {
public:
  virtual ~IThreading() = default;
  virtual std::future<void> enqueue(std::function<void()> task) = 0;
};

class WorkerPool final : public IThreading {
public:
  explicit WorkerPool(std::size_t workerCount = 1);
  ~WorkerPool();

  std::future<void> enqueue(std::function<void()> task) override;

private:
  void workerMain();

private:
  std::mutex mutex_;
  std::condition_variable cv_;
  std::queue<std::packaged_task<void()>> tasks_;
  std::vector<std::thread> workers_;
  bool stopping_ = false;
};

class IPluginLoader {
public:
  virtual ~IPluginLoader() = default;
  virtual bool load(const std::string& path, PluginLoadMode mode) = 0;
  virtual void* symbol(const std::string& name) = 0;
  virtual void unload() = 0;
};

class DynamicPluginLoader final : public IPluginLoader {
public:
  ~DynamicPluginLoader() override;

  bool load(const std::string& path, PluginLoadMode mode) override;
  void* symbol(const std::string& name) override;
  void unload() override;

private:
  void* handle_ = nullptr;
};

class ICrashReporter {
public:
  virtual ~ICrashReporter() = default;
  virtual void install() = 0;
  virtual void capture(const CrashContext& context) = 0;
  virtual std::optional<CrashContext> lastCrash() const = 0;
};

class CrashReporter final : public ICrashReporter {
public:
  void install() override;
  void capture(const CrashContext& context) override;
  std::optional<CrashContext> lastCrash() const override;

private:
  mutable std::mutex mutex_;
  std::optional<CrashContext> lastCrash_;
};

}  // namespace engine::platform
