#include "engine/tools/AssetIO.h"

#include <fstream>

namespace engine::tools {

nlohmann::ordered_json loadJson(const std::string& path) {
  std::ifstream file(path);
  nlohmann::ordered_json json;
  file >> json;
  return json;
}

}
