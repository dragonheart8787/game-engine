#include "weavebound/game/save_game_v0.hpp"

#include <cmath>

int main() {
  using namespace weavebound::game::save_v0;

  PlayerChunk p{};
  p.health = 72.5f;
  p.level = 3;
  p.pos_x = 12.25f;
  p.pos_y = -3.5f;
  p.level_id = 42u;

  const auto blob = encode(p);
  const auto dec = decode(blob);
  if (!dec) {
    return 1;
  }
  if (std::fabs(dec->player.health - 72.5f) > 1e-5f || dec->player.level != 3u) {
    return 2;
  }
  if (std::fabs(dec->player.pos_x - 12.25f) > 1e-5f || std::fabs(dec->player.pos_y - (-3.5f)) > 1e-5f ||
      dec->player.level_id != 42u) {
    return 7;
  }

  WorldFlagsChunk w{};
  w.pairs.push_back({1u, 10u});
  w.pairs.push_back({2u, 20u});
  QuestChunk q{};
  q.active_quest_id = 7u;
  q.step_index = 2u;
  const auto blob2 = encode(p, w, &q);
  const auto dec2 = decode(blob2);
  if (!dec2 || dec2->world.pairs.size() != 2u || !dec2->has_quest) {
    return 3;
  }
  if (dec2->world.pairs[0].first != 1u || dec2->world.pairs[1].second != 20u) {
    return 4;
  }
  if (dec2->quest.active_quest_id != 7u || dec2->quest.step_index != 2u) {
    return 8;
  }

  auto bad = blob;
  if (bad.size() > 16) {
    bad[16] ^= 0xFFu;
  }
  if (decode(bad)) {
    return 5;
  }

  bad[0] ^= 0xFFu;
  if (decode(bad)) {
    return 6;
  }

  return 0;
}
