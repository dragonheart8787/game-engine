#include "weavebound/audio/mixer.hpp"

int main() {
  auto m = weavebound::audio::create_miniaudio_mixer();
  if (!m) {
    return 0;
  }
  m->pump();
  return 0;
}
