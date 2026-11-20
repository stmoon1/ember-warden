#pragma once
#include <memory>
namespace citadel {
class Sound {
  struct Mixer;
  std::unique_ptr<Mixer> mixer;

public:
  float volume = .45f;
  Sound();
  ~Sound();
  void play(int kind);
};
} // namespace citadel
