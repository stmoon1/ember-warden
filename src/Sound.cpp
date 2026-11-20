#include "Sound.h"
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <mutex>
#include <thread>
#include <vector>
#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <mmsystem.h>
#endif
namespace citadel {
struct Sound::Mixer {
#ifdef _WIN32
  struct Voice {
    int kind;
    float age = 0, phase = 0, gain = .4f;
  };
  HWAVEOUT output = nullptr;
  std::array<std::array<short, 256>, 4> samples{};
  std::array<WAVEHDR, 4> buffers{};
  std::vector<Voice> voices;
  std::mutex mutex;
  std::atomic<bool> running{false};
  std::thread worker;
  unsigned noise = 174927;
  Mixer() {
    WAVEFORMATEX format{};
    format.wFormatTag = WAVE_FORMAT_PCM;
    format.nChannels = 1;
    format.nSamplesPerSec = 22050;
    format.wBitsPerSample = 16;
    format.nBlockAlign = 2;
    format.nAvgBytesPerSec = 44100;
    if (waveOutOpen(&output, WAVE_MAPPER, &format, 0, 0, CALLBACK_NULL) !=
        MMSYSERR_NOERROR) {
      output = nullptr;
      return;
    }
    for (size_t i = 0; i < buffers.size(); ++i) {
      buffers[i].lpData = reinterpret_cast<LPSTR>(samples[i].data());
      buffers[i].dwBufferLength = DWORD(samples[i].size() * sizeof(short));
      waveOutPrepareHeader(output, &buffers[i], sizeof(WAVEHDR));
    }
    running = true;
    worker = std::thread([this] {
      while (running) {
        for (size_t block = 0; block < buffers.size(); ++block) {
          auto &buffer = buffers[block];
          if ((buffer.dwFlags & WHDR_INQUEUE) != 0)
            continue;
          std::lock_guard<std::mutex> guard(mutex);
          for (auto &sample : samples[block]) {
            float mixed = 0;
            for (auto &voice : voices) {
              float duration = voice.kind == 8   ? .28f
                               : voice.kind == 7 ? .16f
                                                 : .11f;
              float progress = voice.age / duration;
              if (progress >= 1)
                continue;
              noise ^= noise << 13;
              noise ^= noise >> 17;
              noise ^= noise << 5;
              float grain = float(noise & 65535) / 32768 - 1;
              float frequency = voice.kind == 8   ? 110
                                : voice.kind == 7 ? 280
                                : voice.kind == 0 ? 1800
                                : voice.kind == 3 ? 95
                                : voice.kind == 4 ? 130
                                                  : 650;
              voice.phase += frequency * (1 - progress * .75f) / 22050.f;
              float tone = sin(voice.phase * 6.2831853f);
              float envelope = std::pow(1 - progress, 3.f);
              mixed += (grain * .7f + tone * .3f) * envelope * voice.gain;
              voice.age += 1.f / 22050;
            }
            sample = short(std::clamp(mixed, -1.f, 1.f) * 25000);
          }
          voices.erase(
              std::remove_if(voices.begin(), voices.end(),
                             [](const Voice &v) { return v.age > .3f; }),
              voices.end());
          waveOutWrite(output, &buffer, sizeof(buffer));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(3));
      }
    });
  }
  ~Mixer() {
    running = false;
    if (worker.joinable())
      worker.join();
    if (output) {
      waveOutReset(output);
      for (auto &buffer : buffers)
        waveOutUnprepareHeader(output, &buffer, sizeof(buffer));
      waveOutClose(output);
    }
  }
  void add(int kind, float volume) {
    if (!output)
      return;
    std::lock_guard<std::mutex> guard(mutex);
    if (voices.size() < 24)
      voices.push_back({kind, 0, 0, volume});
  }
#else
  void add(int, float) {}
#endif
};
Sound::Sound() : mixer(std::make_unique<Mixer>()) {}
Sound::~Sound() = default;
void Sound::play(int kind) { mixer->add(kind, volume); }
} // namespace citadel

