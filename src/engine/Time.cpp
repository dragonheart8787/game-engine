#include "engine/Time.h"

void Time::reset(std::uint64_t counter) {
    lastCounter_ = counter;
    deltaSeconds_ = 0.0f;
    totalSeconds_ = 0.0f;
}

void Time::update(std::uint64_t counter, std::uint64_t frequency) {
    const float delta = static_cast<float>(counter - lastCounter_) /
        static_cast<float>(frequency);
    deltaSeconds_ = delta;
    totalSeconds_ += delta;
    lastCounter_ = counter;
}
