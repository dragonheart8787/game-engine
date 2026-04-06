#include "play_session.hpp"
#include "prototype_sound.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <sstream>

namespace weavebound::game_prototype {

namespace {
constexpr std::uint32_t kScrapPerKill = 3u;
constexpr std::uint32_t kScrapSpendCost = 5u;
constexpr float kFocusRestorePerSpend = 15.f;
constexpr std::uint32_t kScrapMaxStored = 999999u;
}  // namespace

void PlaySession::configure_abilities(const AbilitySliceRuntime& spec) {
  spec_ = spec;
}

void PlaySession::set_m1_phase_lines(std::vector<std::string> objectives,
                                    std::vector<std::string> hints) {
  m1_objectives_ = std::move(objectives);
  m1_hints_ = std::move(hints);
}

std::string PlaySession::objective_line() const {
  if (multi_stage_ && !m1_objectives_.empty() &&
      campaign_stage_ >= 0 && campaign_stage_ < static_cast<int>(m1_objectives_.size())) {
    return m1_objectives_[static_cast<std::size_t>(campaign_stage_)];
  }
  if (!m1_objectives_.empty()) {
    return m1_objectives_[0];
  }
  return "前往當前終點信標";
}

std::string PlaySession::hint_line() const {
  if (multi_stage_ && !m1_hints_.empty() && campaign_stage_ >= 0 &&
      campaign_stage_ < static_cast<int>(m1_hints_.size())) {
    return m1_hints_[static_cast<std::size_t>(campaign_stage_)];
  }
  if (!m1_hints_.empty()) {
    return m1_hints_[0];
  }
  return "WASD 移動 | E 扇形技 | Shift+方向 衝刺 | C 碎片換 Focus | P 暫停";
}

void PlaySession::reset_ability_state() {
  focus_ = spec_.focus_max;
  dash_cd_rem_ = 0.f;
  dash_rem_ = 0.f;
  dash_dir_x_ = 0.f;
  dash_dir_y_ = 1.f;
  primary_phase_ = PrimaryPhase::Idle;
  primary_timer_ = 0.f;
  facing_yaw_ = 0.f;
}

void PlaySession::reset_new_game() {
  multi_stage_ = false;
  campaign_stage_ = 0;
  px_ = 3.f;
  py_ = 15.f;
  health_ = 100.f;
  invuln_remaining_ = 0.f;
  autopilot_ = false;
  enemies_.clear();
  projectiles_.clear();
  goal_x_ = 24.f;
  goal_y_ = 15.f;
  goal_radius_ = 2.f;
  scrap_ = 0;
  reset_ability_state();
  spawn_default_encounters();
}

void PlaySession::reset_from_save(const weavebound::game::save_v0::PlayerChunk& p) {
  reset_new_game();
  apply_save(p);
}

void PlaySession::spawn_default_encounters() {
  EnemyActor m{};
  m.kind = EnemyKind::Melee;
  m.x = 11.f;
  m.y = 22.f;
  m.hp = 50.f;
  enemies_.push_back(m);

  EnemyActor r{};
  r.kind = EnemyKind::Ranged;
  r.x = 22.f;
  r.y = 9.f;
  r.hp = 35.f;
  r.attack_cd = 0.5f;
  enemies_.push_back(r);
}

float PlaySession::dist_sq(float ax, float ay, float bx, float by) const {
  const float dx = ax - bx;
  const float dy = ay - by;
  return dx * dx + dy * dy;
}

void PlaySession::try_damage_player(float amount) {
  if (invuln_remaining_ > 0.f || amount <= 0.f) {
    return;
  }
  health_ -= amount;
  if (health_ < 0.f) {
    health_ = 0.f;
  }
  invuln_remaining_ = 0.65f;
  prototype_sound::ping_alert();
}

bool PlaySession::enemy_in_wedge(const EnemyActor& e) const {
  if (e.state == EnemyAiState::Dead || e.hp <= 0.f) {
    return false;
  }
  const float dx = e.x - px_;
  const float dy = e.y - py_;
  const float dist = std::sqrt(std::max(1e-8f, dx * dx + dy * dy));
  if (dist > spec_.primary_range || dist < 1e-4f) {
    return false;
  }
  const float fx = std::sin(facing_yaw_);
  const float fy = std::cos(facing_yaw_);
  const float nx = dx / dist;
  const float ny = dy / dist;
  const float dot = std::clamp(fx * nx + fy * ny, -1.f, 1.f);
  const float ang = std::acos(dot);
  const float half_rad = spec_.primary_half_angle_deg * 3.14159265f / 180.f;
  return ang <= half_rad + 0.02f;
}

void PlaySession::apply_primary_wedge_damage() {
  for (auto& e : enemies_) {
    if (enemy_in_wedge(e)) {
      e.hp -= spec_.primary_damage;
      if (e.hp <= 0.f) {
        e.hp = 0.f;
        e.state = EnemyAiState::Dead;
        scrap_ = std::min(scrap_ + kScrapPerKill, kScrapMaxStored);
      }
    }
  }
  prototype_sound::ping_light();
}

void PlaySession::debug_spawn_enemy_at(float x, float y, float hp) {
  EnemyActor m{};
  m.kind = EnemyKind::Melee;
  m.x = x;
  m.y = y;
  m.hp = hp;
  enemies_.push_back(m);
}

void PlaySession::camera_offset(float* out_cx, float* out_cy, float distance) const {
  const float fx = std::sin(facing_yaw_);
  const float fy = std::cos(facing_yaw_);
  *out_cx = px_ - fx * distance;
  *out_cy = py_ - fy * distance;
}

void PlaySession::player_lit_cube_translate(float out[3]) const {
  out[0] = (px_ - kLitGridCenter) * kLitWorldScale;
  out[1] = 0.f;
  out[2] = (py_ - kLitGridCenter) * kLitWorldScale;
}

void PlaySession::player_lit_look_at(float out[3]) const {
  out[0] = (px_ - kLitGridCenter) * kLitWorldScale;
  out[1] = 0.4f;
  out[2] = (py_ - kLitGridCenter) * kLitWorldScale;
}

void PlaySession::update_enemies(float dt) {
  for (auto& e : enemies_) {
    if (e.state == EnemyAiState::Dead || e.hp <= 0.f) {
      e.state = EnemyAiState::Dead;
      continue;
    }

    const float ddx = px_ - e.x;
    const float ddy = py_ - e.y;
    const float dist = std::sqrt(std::max(1e-6f, dist_sq(e.x, e.y, px_, py_)));

    if (e.kind == EnemyKind::Melee) {
      if (e.state == EnemyAiState::AttackWindup) {
        e.windup -= dt;
        if (e.windup <= 0.f) {
          if (dist <= kMeleeRange + 0.2f) {
            try_damage_player(18.f);
          }
          e.attack_cd = 0.9f;
          e.state = EnemyAiState::Chase;
        }
        continue;
      }
      if (e.attack_cd > 0.f) {
        e.attack_cd -= dt;
      }
      if (dist <= kMeleeRange && e.attack_cd <= 0.f) {
        e.state = EnemyAiState::AttackWindup;
        e.windup = 0.25f;
        continue;
      }
      const float nx = ddx / dist;
      const float ny = ddy / dist;
      e.x += nx * kMeleeSpeed * dt;
      e.y += ny * kMeleeSpeed * dt;
      e.state = EnemyAiState::Chase;
    } else {
      if (e.attack_cd > 0.f) {
        e.attack_cd -= dt;
      }
      if (dist > kRangedOptimal) {
        const float nx = ddx / dist;
        const float ny = ddy / dist;
        e.x += nx * (kMeleeSpeed * 0.65f) * dt;
        e.y += ny * (kMeleeSpeed * 0.65f) * dt;
      }
      if (e.attack_cd <= 0.f && dist < 18.f) {
        const float nx = ddx / dist;
        const float ny = ddy / dist;
        Projectile pr{};
        pr.x = e.x;
        pr.y = e.y;
        pr.vx = nx * kProjectileSpeed;
        pr.vy = ny * kProjectileSpeed;
        projectiles_.push_back(pr);
        e.attack_cd = 1.35f;
      }
    }
  }

  for (auto& pr : projectiles_) {
    pr.x += pr.vx * dt;
    pr.y += pr.vy * dt;
    pr.life_s -= dt;
    if (pr.life_s > 0.f && dist_sq(pr.x, pr.y, px_, py_) < 0.49f) {
      try_damage_player(12.f);
      pr.life_s = -1.f;
    }
  }
  projectiles_.erase(
      std::remove_if(projectiles_.begin(), projectiles_.end(),
                     [](const Projectile& p) { return p.life_s <= 0.f; }),
      projectiles_.end());
}

SessionOutcome PlaySession::update(float dt, const SessionInput& in) {
  if (invuln_remaining_ > 0.f) {
    invuln_remaining_ -= dt;
  }

  if (dash_cd_rem_ > 0.f) {
    dash_cd_rem_ -= dt;
  }
  if (dash_rem_ > 0.f) {
    dash_rem_ -= dt;
  }

  float mx = in.move_x;
  float my = in.move_y;
  if (autopilot_) {
    const float gdx = goal_x_ - px_;
    const float gdy = goal_y_ - py_;
    const float gl = std::sqrt(std::max(1e-8f, gdx * gdx + gdy * gdy));
    mx = gdx / gl;
    my = gdy / gl;
  }

  if (std::abs(in.aim_delta_x) > 0.0001f) {
    facing_yaw_ += in.aim_delta_x * kYawMouseScale;
  } else if (!autopilot_ && (mx * mx + my * my > 1e-5f)) {
    facing_yaw_ = std::atan2(mx, my);
  }

  if (primary_phase_ == PrimaryPhase::Windup) {
    primary_timer_ -= dt;
    if (primary_timer_ <= 0.f) {
      primary_phase_ = PrimaryPhase::Active;
      primary_timer_ = spec_.primary_active_s;
      apply_primary_wedge_damage();
    }
  } else if (primary_phase_ == PrimaryPhase::Active) {
    primary_timer_ -= dt;
    if (primary_timer_ <= 0.f) {
      primary_phase_ = PrimaryPhase::Idle;
      primary_timer_ = 0.f;
    }
  }

  if (primary_phase_ == PrimaryPhase::Idle && in.primary_pressed) {
    if (focus_ >= static_cast<float>(spec_.cost_amount)) {
      focus_ -= static_cast<float>(spec_.cost_amount);
      primary_phase_ = PrimaryPhase::Windup;
      primary_timer_ = spec_.primary_windup_s;
    }
  }

  if (in.dash_pressed && dash_cd_rem_ <= 0.f && dash_rem_ <= 0.f && !autopilot_) {
    const float mlen = std::sqrt(mx * mx + my * my);
    if (mlen > 1e-4f) {
      dash_dir_x_ = mx / mlen;
      dash_dir_y_ = my / mlen;
      dash_rem_ = spec_.dash_duration_s;
      dash_cd_rem_ = spec_.dash_cooldown_s;
      invuln_remaining_ = std::max(invuln_remaining_, spec_.dash_iframes_s);
    }
  }

  if (in.consume_pressed && !autopilot_ && scrap_ >= kScrapSpendCost) {
    scrap_ -= kScrapSpendCost;
    focus_ = std::min(focus_ + kFocusRestorePerSpend, spec_.focus_max);
    prototype_sound::ping_light();
  }

  float move_dx = mx;
  float move_dy = my;
  if (dash_rem_ > 0.f) {
    move_dx = dash_dir_x_;
    move_dy = dash_dir_y_;
  }

  const float speed = (dash_rem_ > 0.f) ? (kPlayerSpeed * spec_.dash_speed_mult) : kPlayerSpeed;
  px_ += move_dx * speed * dt;
  py_ += move_dy * speed * dt;
  px_ = std::clamp(px_, 0.f, 28.f);
  py_ = std::clamp(py_, 0.f, 28.f);

  update_enemies(dt);

  if (health_ <= 0.f) {
    return SessionOutcome::Defeat;
  }
  if (dist_sq(px_, py_, goal_x_, goal_y_) <= goal_radius_ * goal_radius_) {
    if (multi_stage_ && campaign_stage_ < 2) {
      setup_campaign_stage(campaign_stage_ + 1);
      prototype_sound::ping_light();
      reset_ability_state();
      return SessionOutcome::Ongoing;
    }
    return SessionOutcome::Victory;
  }
  return SessionOutcome::Ongoing;
}

void PlaySession::set_multi_stage_campaign(bool on) {
  multi_stage_ = on;
  scrap_ = 0;
  health_ = 100.f;
  invuln_remaining_ = 0.f;
  autopilot_ = false;
  enemies_.clear();
  projectiles_.clear();
  if (on) {
    setup_campaign_stage(0);
  } else {
    campaign_stage_ = 0;
    px_ = 3.f;
    py_ = 15.f;
    goal_x_ = 24.f;
    goal_y_ = 15.f;
    goal_radius_ = 2.f;
    reset_ability_state();
    spawn_default_encounters();
  }
}

void PlaySession::setup_campaign_stage(int stage) {
  campaign_stage_ = stage;
  enemies_.clear();
  projectiles_.clear();
  reset_ability_state();
  if (stage == 0) {
    goal_x_ = 24.f;
    goal_y_ = 15.f;
    goal_radius_ = 2.f;
    px_ = 3.f;
    py_ = 15.f;
    spawn_default_encounters();
  } else if (stage == 1) {
    goal_x_ = 25.f;
    goal_y_ = 22.f;
    goal_radius_ = 2.f;
    px_ = 5.f;
    py_ = 5.f;
    EnemyActor m{};
    m.kind = EnemyKind::Melee;
    m.x = 18.f;
    m.y = 12.f;
    m.hp = 48.f;
    enemies_.push_back(m);
    EnemyActor r{};
    r.kind = EnemyKind::Ranged;
    r.x = 8.f;
    r.y = 18.f;
    r.hp = 32.f;
    r.attack_cd = 0.5f;
    enemies_.push_back(r);
  } else if (stage == 2) {
    goal_x_ = 14.f;
    goal_y_ = 8.f;
    goal_radius_ = 1.8f;
    px_ = 14.f;
    py_ = 22.f;
    EnemyActor boss{};
    boss.kind = EnemyKind::Melee;
    boss.x = 14.f;
    boss.y = 11.f;
    boss.hp = 92.f;
    enemies_.push_back(boss);
  }
}

void PlaySession::apply_save(const weavebound::game::save_v0::PlayerChunk& p) {
  health_ = p.health;
  px_ = p.pos_x;
  py_ = p.pos_y;
  reset_ability_state();
}

void PlaySession::apply_world_flags(const weavebound::game::save_v0::WorldFlagsChunk& w) {
  scrap_ = 0;
  for (const auto& pr : w.pairs) {
    if (pr.first == weavebound::game::save_v0::kWorldKeyPrototypeScrap) {
      scrap_ = std::min(pr.second, kScrapMaxStored);
      return;
    }
  }
}

weavebound::game::save_v0::WorldFlagsChunk PlaySession::build_world_flags() const {
  using weavebound::game::save_v0::WorldFlagsChunk;
  WorldFlagsChunk w{};
  w.pairs.push_back(
      std::make_pair(weavebound::game::save_v0::kWorldKeyPrototypeScrap, scrap_));
  return w;
}

weavebound::game::save_v0::PlayerChunk PlaySession::build_player_chunk() const {
  weavebound::game::save_v0::PlayerChunk p{};
  p.health = health_;
  p.level = 1;
  p.pos_x = px_;
  p.pos_y = py_;
  p.level_id = 1;
  return p;
}

std::string PlaySession::hud_line() const {
  float ccx = 0.f;
  float ccy = 0.f;
  camera_offset(&ccx, &ccy, 4.f);
  std::ostringstream os;
  os << "HP " << static_cast<int>(health_) << " | Fcs " << static_cast<int>(focus_) << "/"
     << static_cast<int>(spec_.focus_max) << " | Scrap " << scrap_ << " | Yaw "
     << static_cast<int>(facing_yaw_ * 57.29578f)
     << " | Cam(" << static_cast<int>(ccx) << "," << static_cast<int>(ccy) << ") | Goal ("
     << static_cast<int>(goal_x_) << "," << static_cast<int>(goal_y_) << ") | [V]音效";
  if (multi_stage_) {
    os << " | 段落 " << (campaign_stage_ + 1) << "/3";
  }
  os << " | ";
  os << objective_line();
  os << " WASD E技 Shift衝 C碎片換Focus P pause Space ack";
  return os.str();
}

}  // namespace weavebound::game_prototype
