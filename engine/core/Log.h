#pragma once

#include <string>

namespace engine::core {

void logInfo(const std::string& message);
void logWarning(const std::string& message);
void logError(const std::string& message);

}  // namespace engine::core
