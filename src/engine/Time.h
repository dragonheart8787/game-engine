#pragma once

#include <cstdint>

class Time {
public:
    void reset(std::uint64_t counter);
    void update(std::uint64_t counter, std::uint64_t frequency);

    float deltaSeconds() const { return deltaSeconds_; }
    float totalSeconds() const { return totalSeconds_; }

private:
    std::uint64_t lastCounter_ = 0;
    float deltaSeconds_ = 0.0f;
    float totalSeconds_ = 0.0f;
};
