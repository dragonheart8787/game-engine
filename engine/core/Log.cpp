#include "engine/core/Log.h"

#include <iostream>

namespace engine::core {

void logInfo(const std::string& message) {
  std::cout << "[Info] " << message << '\n';
}

void logWarning(const std::string& message) {
  std::cout << "[Warn] " << message << '\n';
}

void logError(const std::string& message) {
  std::cerr << "[Error] " << message << '\n';
}

}  // namespace engine::core
