#pragma once

namespace weavebound::version {

constexpr int kMajor = 0;
constexpr int kMinor = 2;
constexpr int kPatch = 3;

/** Milestone tag; bump when §8 M0→M1… 交付邊界變更 */
constexpr const char* kMilestone = "M1";

inline constexpr const char* string() { return "0.2.3-m1"; }

}  // namespace weavebound::version
