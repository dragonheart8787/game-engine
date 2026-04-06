#include "weavebound/audio/mixer.hpp"

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

namespace weavebound::audio {

namespace {

class MiniaudioMixer final : public IAudioMixer {
 public:
  MiniaudioMixer() { ok_ = (ma_engine_init(nullptr, &engine_) == MA_SUCCESS); }

  ~MiniaudioMixer() override {
    if (ok_) {
      ma_engine_uninit(&engine_);
    }
  }

  void pump() override {
    // 引擎自帶裝置執行緒；smoke 僅驗證 init／uninit。
  }

  bool ok() const { return ok_; }

 private:
  ma_engine engine_{};
  bool ok_{false};
};

}  // namespace

std::unique_ptr<IAudioMixer> create_miniaudio_mixer() {
  auto m = std::make_unique<MiniaudioMixer>();
  if (!m->ok()) {
    return nullptr;
  }
  return m;
}

}  // namespace weavebound::audio
