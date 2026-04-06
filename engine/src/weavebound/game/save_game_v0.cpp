#include "weavebound/game/save_game_v0.hpp"

#include <cstring>

namespace weavebound::game::save_v0 {

namespace {

void append_u32(std::vector<std::uint8_t>& out, std::uint32_t v) {
  out.push_back(static_cast<std::uint8_t>(v & 0xFFu));
  out.push_back(static_cast<std::uint8_t>((v >> 8) & 0xFFu));
  out.push_back(static_cast<std::uint8_t>((v >> 16) & 0xFFu));
  out.push_back(static_cast<std::uint8_t>((v >> 24) & 0xFFu));
}

bool read_u32(const std::uint8_t*& p, const std::uint8_t* end, std::uint32_t* out) {
  if (end - p < 4) {
    return false;
  }
  *out = static_cast<std::uint32_t>(p[0]) | (static_cast<std::uint32_t>(p[1]) << 8) |
         (static_cast<std::uint32_t>(p[2]) << 16) | (static_cast<std::uint32_t>(p[3]) << 24);
  p += 4;
  return true;
}

}  // namespace

std::uint32_t crc32_ieee(const std::uint8_t* data, std::size_t len) {
  std::uint32_t c = 0xFFFFFFFFu;
  for (std::size_t i = 0; i < len; ++i) {
    c ^= data[i];
    for (int k = 0; k < 8; ++k) {
      const std::uint32_t mask = static_cast<std::uint32_t>(-(static_cast<int>(c & 1u)));
      c = (c >> 1) ^ (0xEDB88320u & mask);
    }
  }
  return ~c;
}

std::vector<std::uint8_t> encode(const PlayerChunk& player, const WorldFlagsChunk& world,
                                   const QuestChunk* quest) {
  std::vector<std::uint8_t> body;
  std::uint32_t chunk_count = 1u;
  if (!world.pairs.empty()) {
    ++chunk_count;
  }
  if (quest != nullptr) {
    ++chunk_count;
  }
  append_u32(body, chunk_count);

  // PLAY chunk（v0 延伸：20 bytes；舊版 8 bytes 仍可由 decode 辨識）
  append_u32(body, kChunkPlayer);
  append_u32(body, 20u);
  std::uint32_t health_bits = 0;
  std::memcpy(&health_bits, &player.health, sizeof(health_bits));
  append_u32(body, health_bits);
  append_u32(body, player.level);
  std::uint32_t pos_x_bits = 0, pos_y_bits = 0;
  std::memcpy(&pos_x_bits, &player.pos_x, sizeof(pos_x_bits));
  std::memcpy(&pos_y_bits, &player.pos_y, sizeof(pos_y_bits));
  append_u32(body, pos_x_bits);
  append_u32(body, pos_y_bits);
  append_u32(body, player.level_id);

  if (!world.pairs.empty()) {
    append_u32(body, kChunkWorldFlags);
    const std::uint32_t wc = static_cast<std::uint32_t>(world.pairs.size());
    const std::uint32_t psz = 4u + wc * 8u;
    append_u32(body, psz);
    append_u32(body, wc);
    for (const auto& pr : world.pairs) {
      append_u32(body, pr.first);
      append_u32(body, pr.second);
    }
  }

  if (quest != nullptr) {
    append_u32(body, kChunkQuestStub);
    append_u32(body, 8u);
    append_u32(body, quest->active_quest_id);
    append_u32(body, quest->step_index);
  }

  std::vector<std::uint8_t> file;
  file.reserve(16 + body.size());
  append_u32(file, kMagic);
  append_u32(file, kFormatVersion);
  append_u32(file, 0u);  // flags
  const std::uint32_t crc = crc32_ieee(body.data(), body.size());
  append_u32(file, crc);
  file.insert(file.end(), body.begin(), body.end());
  return file;
}

std::optional<DecodedSave> decode(const std::uint8_t* data, std::size_t size) {
  if (size < 16) {
    return std::nullopt;
  }
  const std::uint8_t* p = data;
  const std::uint8_t* end = data + size;
  std::uint32_t magic = 0, ver = 0, flags = 0, crc_stored = 0;
  if (!read_u32(p, end, &magic) || !read_u32(p, end, &ver) || !read_u32(p, end, &flags) ||
      !read_u32(p, end, &crc_stored)) {
    return std::nullopt;
  }
  if (magic != kMagic || ver != kFormatVersion) {
    return std::nullopt;
  }
  const std::size_t body_len = static_cast<std::size_t>(end - p);
  if (body_len == 0) {
    return std::nullopt;
  }
  const std::uint32_t crc_calc = crc32_ieee(p, body_len);
  if (crc_calc != crc_stored) {
    return std::nullopt;
  }

  DecodedSave out{};
  std::uint32_t chunk_count = 0;
  if (!read_u32(p, end, &chunk_count)) {
    return std::nullopt;
  }
  for (std::uint32_t i = 0; i < chunk_count; ++i) {
    std::uint32_t id = 0, psz = 0;
    if (!read_u32(p, end, &id) || !read_u32(p, end, &psz)) {
      return std::nullopt;
    }
    if (psz > static_cast<std::uint32_t>(end - p)) {
      return std::nullopt;
    }
    const std::uint8_t* payload = p;
    p += psz;

    if (id == kChunkPlayer) {
      if (psz < 8) {
        return std::nullopt;
      }
      std::uint32_t hf = 0, lv = 0;
      const std::uint8_t* q = payload;
      if (!read_u32(q, payload + psz, &hf) || !read_u32(q, payload + psz, &lv)) {
        return std::nullopt;
      }
      std::memcpy(&out.player.health, &hf, sizeof(out.player.health));
      out.player.level = lv;
      if (psz >= 20) {
        std::uint32_t px = 0, py = 0, lid = 0;
        if (!read_u32(q, payload + psz, &px) || !read_u32(q, payload + psz, &py) ||
            !read_u32(q, payload + psz, &lid)) {
          return std::nullopt;
        }
        std::memcpy(&out.player.pos_x, &px, sizeof(out.player.pos_x));
        std::memcpy(&out.player.pos_y, &py, sizeof(out.player.pos_y));
        out.player.level_id = lid;
      }
    } else if (id == kChunkQuestStub) {
      if (psz < 8) {
        return std::nullopt;
      }
      const std::uint8_t* q = payload;
      std::uint32_t aq = 0, st = 0;
      if (!read_u32(q, payload + psz, &aq) || !read_u32(q, payload + psz, &st)) {
        return std::nullopt;
      }
      out.has_quest = true;
      out.quest.active_quest_id = aq;
      out.quest.step_index = st;
    } else if (id == kChunkWorldFlags) {
      if (psz < 4) {
        return std::nullopt;
      }
      const std::uint8_t* q = payload;
      std::uint32_t pairs = 0;
      if (!read_u32(q, payload + psz, &pairs)) {
        return std::nullopt;
      }
      if (psz != 4u + pairs * 8u) {
        return std::nullopt;
      }
      out.world.pairs.clear();
      out.world.pairs.reserve(pairs);
      for (std::uint32_t j = 0; j < pairs; ++j) {
        std::uint32_t k = 0, v = 0;
        if (!read_u32(q, payload + psz, &k) || !read_u32(q, payload + psz, &v)) {
          return std::nullopt;
        }
        out.world.pairs.emplace_back(k, v);
      }
    }
  }
  if (p != end) {
    return std::nullopt;
  }
  return out;
}

}  // namespace weavebound::game::save_v0
