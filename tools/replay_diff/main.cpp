#include <fstream>
#include <iostream>

#include <nlohmann/json.hpp>

int main(int argc, char** argv) {
  if (argc < 3) {
    std::cerr << "usage: replay_diff <runA.json> <runB.json>\n";
    return 1;
  }

  nlohmann::ordered_json a;
  nlohmann::ordered_json b;
  std::ifstream(argv[1]) >> a;
  std::ifstream(argv[2]) >> b;

  const auto& framesA = a.value("frames", nlohmann::ordered_json::array());
  const auto& framesB = b.value("frames", nlohmann::ordered_json::array());
  const std::size_t n = std::min(framesA.size(), framesB.size());
  for (std::size_t i = 0; i < n; ++i) {
    if (framesA[i] != framesB[i]) {
      std::cout << "divergence_tick=" << i << " system=input streamId=0 delta_seq=" << i << "\n";
      return 2;
    }
  }
  if (framesA.size() != framesB.size()) {
    std::cout << "divergence_tick=" << n << " system=input streamId=0 delta_seq=" << n << "\n";
    return 2;
  }
  std::cout << "replay identical\n";
  return 0;
}
