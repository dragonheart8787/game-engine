#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

namespace weavebound::game::save_v0 {

/** 檔案 magic「WBVS」（WeaveBound Save）；小端序 uint32 與 TDD 一致。 */
constexpr std::uint32_t kMagic = 0x53564257u;

constexpr std::uint32_t kFormatVersion = 1;

constexpr std::uint32_t kChunkPlayer = 0x504C4159u;  // 'PLAY' (big-endian style ID)
constexpr std::uint32_t kChunkWorldFlags = 0x574F524Cu;  // 'WORL'
constexpr std::uint32_t kChunkQuestStub = 0x51535453u;   // 'QSTS'

/** WORL pair 鍵：原型碎片計數（見 SAVE_CONTRACT_V1、PlaySession::build_world_flags）。值 clamp ≤ 999999。 */
constexpr std::uint32_t kWorldKeyPrototypeScrap = 0x53435250u;  // 'SCRP' big-endian style

struct PlayerChunk {
  float health = 100.f;
  std::uint32_t level = 1;
  /** 關卡內位置（原型／2D 模擬層）。 */
  float pos_x = 0.f;
  float pos_y = 0.f;
  /** 場景或關卡 id（對齊 TDD 進度邊界）。 */
  std::uint32_t level_id = 0;
};

struct WorldFlagsChunk {
  std::vector<std::pair<std::uint32_t, std::uint32_t>> pairs;
};

/** 任務占位（TDD `QSTS`）；Vertical Slice 擴充欄位。 */
struct QuestChunk {
  std::uint32_t active_quest_id = 0;
  std::uint32_t step_index = 0;
};

struct DecodedSave {
  PlayerChunk player{};
  WorldFlagsChunk world{};
  bool has_quest{false};
  QuestChunk quest{};
};

/** CRC-32 / IEEE（與 PNG／乙太網路相同多項式），用於檔頭校驗欄位。 */
std::uint32_t crc32_ieee(const std::uint8_t* data, std::size_t len);

std::vector<std::uint8_t> encode(const PlayerChunk& player,
                                   const WorldFlagsChunk& world = {},
                                   const QuestChunk* quest = nullptr);

/** 解碼失敗回傳 nullopt（magic／版本／CRC／chunk 長度錯誤）。 */
std::optional<DecodedSave> decode(const std::uint8_t* data, std::size_t size);

inline std::optional<DecodedSave> decode(const std::vector<std::uint8_t>& blob) {
  return decode(blob.data(), blob.size());
}

}  // namespace weavebound::game::save_v0
