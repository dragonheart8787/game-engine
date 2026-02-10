#pragma once

#include <nlohmann/json.hpp>
#include <string>

namespace engine::tools {

nlohmann::ordered_json loadJson(const std::string& path);

}
