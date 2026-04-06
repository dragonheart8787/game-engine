#pragma once

#include <memory>

namespace weavebound::audio {

/** Mixer、3D spatial、streaming（規格 1.8；建議 miniaudio）。 */
class IAudioMixer {
 public:
  virtual ~IAudioMixer() = default;
  virtual void pump() = 0;
};

/** 以 miniaudio `ma_engine` 建立（失敗時回傳 nullptr）。 */
std::unique_ptr<IAudioMixer> create_miniaudio_mixer();

}  // namespace weavebound::audio
