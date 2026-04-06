#pragma once

#include <string>

namespace weavebound::game_prototype {

/** 與 eidrix_mvp.ability_kernel.spec_v0.SCHEMA_VERSION 一致 */
inline constexpr const char* kAbilitySliceSchemaVersion = "ability_slice_v0";

struct AbilitySliceRuntime {
  std::string schema_version{kAbilitySliceSchemaVersion};
  std::string id{"slice_default"};
  float focus_max{100.f};

  float dash_cooldown_s{1.2f};
  float dash_duration_s{0.18f};
  float dash_speed_mult{3.5f};
  float dash_iframes_s{0.15f};

  float primary_range{3.5f};
  float primary_half_angle_deg{55.f};
  float primary_windup_s{0.12f};
  float primary_active_s{0.08f};
  float primary_damage{28.f};

  int cost_amount{39};
};

}  // namespace weavebound::game_prototype
