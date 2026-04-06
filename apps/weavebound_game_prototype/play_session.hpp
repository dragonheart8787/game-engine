#pragma once

#include "ability_spec.hpp"
#include "weavebound/game/save_game_v0.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace weavebound::game_prototype {

enum class SessionOutcome {
  Ongoing,
  Victory,
  Defeat,
};

enum class EnemyKind { Melee, Ranged };

enum class EnemyAiState { Idle, Chase, AttackWindup, Dead };

struct Projectile {
  float x{};
  float y{};
  float vx{};
  float vy{};
  float life_s{3.f};
};

struct EnemyActor {
  EnemyKind kind{EnemyKind::Melee};
  EnemyAiState state{EnemyAiState::Idle};
  float x{};
  float y{};
  float hp{40.f};
  float attack_cd{0.f};
  float windup{0.f};
};

/** 每幀輸入：移動、能力邊緣觸發、滑鼠水平增量（第三人稱偏航）。 */
struct SessionInput {
  float move_x{0.f};
  float move_y{0.f};
  bool dash_pressed{false};
  bool primary_pressed{false};
  /** C 鍵邊緣：消耗碎片換 Focus（見 PlaySession 常數）。 */
  bool consume_pressed{false};
  float aim_delta_x{0.f};
};

enum class PrimaryPhase { Idle, Windup, Active };

/**
 * 可測試的 2D 頂視玩法層：出生點、終點觸發、近戰／遠程敵、受擊無敵、專案驗證用自動走位。
 * 擴充：面向、Dash、資料驅動扇形技、專注資源；之後可由相同規則掛載 ECS。
 */
class PlaySession {
 public:
  void reset_new_game();
  void reset_from_save(const weavebound::game::save_v0::PlayerChunk& p);

  /** 由 ability_slice_v0.json 載入後注入；未呼叫前使用內建預設（與 Eidrix 匯出一致）。 */
  void configure_abilities(const AbilitySliceRuntime& spec);

  /** M1：三段落戰役用的目標／教學字串（來自 level_m1.json）。 */
  void set_m1_phase_lines(std::vector<std::string> objectives, std::vector<std::string> hints);

  std::string objective_line() const;
  std::string hint_line() const;

  SessionOutcome update(float dt, const SessionInput& in);

  /** CI：每幀朝終點方向移動，忽略玩家輸入。 */
  void set_autopilot_to_goal(bool on) { autopilot_ = on; }

  float player_x() const { return px_; }
  float player_y() const { return py_; }
  float player_health() const { return health_; }
  bool player_invuln() const { return invuln_remaining_ > 0.f; }
  float player_facing_yaw() const { return facing_yaw_; }
  float player_focus() const { return focus_; }

  /** 偽第三人稱：相機在玩家後方（平面），供 HUD 顯示。 */
  void camera_offset(float* out_cx, float* out_cy, float distance = 4.f) const;

  /** Lit demo：立方體平移（世界 XZ），Y 由 lit 內建基底高度加上此處 [1]。 */
  void player_lit_cube_translate(float out[3]) const;
  void player_lit_look_at(float out[3]) const;

  void apply_save(const weavebound::game::save_v0::PlayerChunk& p);
  void apply_world_flags(const weavebound::game::save_v0::WorldFlagsChunk& w);
  weavebound::game::save_v0::PlayerChunk build_player_chunk() const;
  weavebound::game::save_v0::WorldFlagsChunk build_world_flags() const;

  std::uint32_t prototype_scrap() const { return scrap_; }

  /** CI／除錯：強制血量與位置。 */
  void debug_set_health(float h) { health_ = h; }
  void debug_set_position(float x, float y) {
    px_ = x;
    py_ = y;
  }
  /** CI：清空威脅，僅驗證到達終點與流程。 */
  void debug_clear_threats() {
    enemies_.clear();
    projectiles_.clear();
  }
  void debug_spawn_enemy_at(float x, float y, float hp = 50.f);

  std::string hud_line() const;
  const std::vector<EnemyActor>& enemies() const { return enemies_; }
  const std::vector<Projectile>& projectiles() const { return projectiles_; }
  float goal_x() const { return goal_x_; }
  float goal_y() const { return goal_y_; }
  float goal_radius() const { return goal_radius_; }

  /** --play：三段落戰役（到達終點後進入下一段，最後一段才 Victory） */
  void set_multi_stage_campaign(bool on);

  int campaign_stage() const { return campaign_stage_; }
  bool multi_stage() const { return multi_stage_; }

 private:
  void reset_ability_state();
  void spawn_default_encounters();
  void setup_campaign_stage(int stage);
  void update_enemies(float dt);
  void try_damage_player(float amount);
  float dist_sq(float ax, float ay, float bx, float by) const;
  bool enemy_in_wedge(const EnemyActor& e) const;
  void apply_primary_wedge_damage();

  float px_{3.f};
  float py_{15.f};
  float health_{100.f};
  float invuln_remaining_{0.f};
  bool autopilot_{false};

  float goal_x_{24.f};
  float goal_y_{15.f};
  float goal_radius_{2.f};

  bool multi_stage_{false};
  int campaign_stage_{0};

  AbilitySliceRuntime spec_{};
  float facing_yaw_{0.f};
  float focus_{100.f};
  float dash_cd_rem_{0.f};
  float dash_rem_{0.f};
  float dash_dir_x_{0.f};
  float dash_dir_y_{1.f};
  PrimaryPhase primary_phase_{PrimaryPhase::Idle};
  float primary_timer_{0.f};

  static constexpr float kPlayerSpeed = 14.f;
  static constexpr float kMeleeSpeed = 5.f;
  static constexpr float kMeleeRange = 1.1f;
  static constexpr float kMeleeDps = 22.f;
  static constexpr float kRangedOptimal = 7.f;
  static constexpr float kProjectileSpeed = 10.f;
  static constexpr float kYawMouseScale = 0.004f;
  static constexpr float kLitWorldScale = 0.08f;
  static constexpr float kLitGridCenter = 14.f;

  std::vector<std::string> m1_objectives_;
  std::vector<std::string> m1_hints_;

  /** 原型碎片（擊殺取得；C 鍵兌換 Focus）；經 WORL 鍵 kWorldKeyPrototypeScrap 持久化。 */
  std::uint32_t scrap_{0};

  std::vector<EnemyActor> enemies_;
  std::vector<Projectile> projectiles_;
};

}  // namespace weavebound::game_prototype
