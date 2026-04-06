#include "weavebound/asset/wbpak.hpp"

#include <cstring>
#include <string_view>

namespace weavebound::asset {

static std::uint64_t fnv1a64(const std::uint8_t* p, std::size_t n) {
  std::uint64_t h = 14695981039346656037ull;
  for (std::size_t i = 0; i < n; ++i) {
    h ^= p[i];
    h *= 1099511628211ull;
  }
  return h;
}

std::vector<std::uint8_t> build_wbpak(const std::vector<WbpakFileEntry>& entries) {
  std::vector<std::uint8_t> out;
  WbpakHeader head{};
  head.entry_count = static_cast<std::uint32_t>(entries.size());
  const std::size_t manifest_bytes = entries.size() * sizeof(ManifestEntry);
  std::vector<std::uint8_t> manifest(manifest_bytes);
  std::memset(manifest.data(), 0, manifest_bytes);

  std::uint64_t off = sizeof(WbpakHeader) + manifest_bytes;
  for (std::size_t i = 0; i < entries.size(); ++i) {
    auto& me = reinterpret_cast<ManifestEntry*>(manifest.data())[i];
    const std::string& name = entries[i].logical_name;
    const std::size_t copy_n = std::min(name.size(), sizeof(me.logical_name) - 1);
    std::memcpy(me.logical_name, name.data(), copy_n);
    me.content_hash = fnv1a64(entries[i].bytes.data(), entries[i].bytes.size());
    me.byte_offset = off;
    me.byte_size = entries[i].bytes.size();
    off += entries[i].bytes.size();
  }

  out.resize(static_cast<std::size_t>(off));
  std::memcpy(out.data(), &head, sizeof(head));
  std::memcpy(out.data() + sizeof(head), manifest.data(), manifest_bytes);
  const auto* mes = reinterpret_cast<const ManifestEntry*>(manifest.data());
  for (std::size_t j = 0; j < entries.size(); ++j) {
    const ManifestEntry& me = mes[j];
    std::memcpy(out.data() + me.byte_offset, entries[j].bytes.data(), entries[j].bytes.size());
  }
  return out;
}

bool wbpak_find(const std::uint8_t* pak_data, std::size_t pak_size, std::string_view logical_name,
                std::size_t& out_offset, std::size_t& out_size) {
  out_offset = 0;
  out_size = 0;
  if (!pak_data || pak_size < sizeof(WbpakHeader)) {
    return false;
  }
  WbpakHeader head{};
  std::memcpy(&head, pak_data, sizeof(head));
  if (head.magic != kWbpakMagic || head.version < 1 || head.entry_count == 0) {
    return false;
  }
  const std::size_t manifest_bytes = static_cast<std::size_t>(head.entry_count) * sizeof(ManifestEntry);
  if (pak_size < sizeof(WbpakHeader) + manifest_bytes) {
    return false;
  }
  const auto* entries = reinterpret_cast<const ManifestEntry*>(pak_data + sizeof(WbpakHeader));
  for (std::uint32_t i = 0; i < head.entry_count; ++i) {
    const ManifestEntry& me = entries[i];
    const char* z = static_cast<const char*>(std::memchr(me.logical_name, '\0', sizeof(me.logical_name)));
    const std::size_t name_len = z ? static_cast<std::size_t>(z - me.logical_name) : sizeof(me.logical_name);
    const std::string_view name(me.logical_name, name_len);
    if (name != logical_name) {
      continue;
    }
    if (me.byte_offset >= pak_size || me.byte_size > pak_size - me.byte_offset) {
      return false;
    }
    out_offset = static_cast<std::size_t>(me.byte_offset);
    out_size = static_cast<std::size_t>(me.byte_size);
    return true;
  }
  return false;
}

}  // namespace weavebound::asset
