#pragma once

namespace weavebound::platform {

/** 高精度計時與幀節奏（規格 §1.1 Time）；與 clock.hpp 互補，供主循環調度。 */
struct FramePacingConfig {
    double target_fps = 60.0;
    bool   vsync_hint = true;
};

class IFramePacer {
public:
    virtual ~IFramePacer() = default;

    virtual void begin_frame() = 0;
    /** 若需睡眠／等待 present，回傳建議 sleep 秒數。 */
    virtual double end_frame_and_sleep_hint() = 0;
};

}  // namespace weavebound::platform
