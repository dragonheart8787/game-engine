#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>

namespace fs = std::filesystem;

namespace {
std::uint64_t fnv1a(const std::vector<std::uint8_t>& data) {
  std::uint64_t hash = 1469598103934665603ull;
  for (auto b : data) {
    hash ^= static_cast<std::uint64_t>(b);
    hash *= 1099511628211ull;
  }
  return hash;
}

std::vector<std::uint8_t> readBytes(const fs::path& path) {
  std::ifstream file(path, std::ios::binary);
  if (!file.is_open()) {
    return {};
  }
  return std::vector<std::uint8_t>((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cerr << "usage: asset_packer <asset_root> <output_manifest.json>\n";
    return 1;
  }

  const fs::path assetRoot = argv[1];
  const fs::path outputPath = argv[2];

  if (!fs::exists(assetRoot)) {
    std::cerr << "asset root does not exist: " << assetRoot << "\n";
    return 1;
  }

  std::vector<fs::path> files;
  for (const auto& entry : fs::recursive_directory_iterator(assetRoot)) {
    if (entry.is_regular_file()) {
      files.push_back(entry.path());
    }
  }
  std::sort(files.begin(), files.end());

  nlohmann::ordered_json manifest;
  manifest["version"] = 1;
  manifest["root"] = assetRoot.generic_string();
  manifest["entries"] = nlohmann::ordered_json::array();

  std::uint64_t globalHash = 1469598103934665603ull;
  std::size_t totalBytes = 0;

  for (const auto& filePath : files) {
    const auto bytes = readBytes(filePath);
    const std::uint64_t fileHash = fnv1a(bytes);
    totalBytes += bytes.size();

    const std::string rel = fs::relative(filePath, assetRoot).generic_string();
    manifest["entries"].push_back({
        {"path", rel},
        {"size", bytes.size()},
        {"hash_fnv1a64", fileHash}
    });

    // deterministic global hash chaining
    for (char c : rel) {
      globalHash ^= static_cast<std::uint64_t>(static_cast<unsigned char>(c));
      globalHash *= 1099511628211ull;
    }
    globalHash ^= fileHash;
    globalHash *= 1099511628211ull;
  }

  manifest["total_files"] = files.size();
  manifest["total_bytes"] = totalBytes;
  manifest["global_hash_fnv1a64"] = globalHash;

  std::ofstream out(outputPath);
  if (!out.is_open()) {
    std::cerr << "cannot write output: " << outputPath << "\n";
    return 1;
  }
  out << manifest.dump(2);

  std::cout << "asset_packer wrote manifest: " << outputPath << "\n";
  return 0;
}
