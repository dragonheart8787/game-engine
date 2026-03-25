#include <iostream>

#include "game_engine/core.hpp"

int main() {
  std::cout << "game_engine_smoke v" << game_engine::core_version_major() << '\n';
  return 0;
}
