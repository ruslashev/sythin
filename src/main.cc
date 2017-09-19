#include <cmath>
#include "wav_writer.hh"
#include "lang.hh"

#define src(X) #X
const char *ex_src = src(
  main f t = sin(2 * pi * f * t)
);

samples_t create_2chan_samples(uint64_t num_samples) {
  samples_t samples = {
    std::vector<uint16_t>(num_samples), std::vector<uint16_t>(num_samples)
  };
  return samples;
}

int main() {
  double max_amplitude = 32760;
  double sample_rate = 44100;
  double frequency = 261.626 /* C4 */, seconds = 2.5;
  uint64_t num_samples = sample_rate * seconds;
  samples_t samples = create_2chan_samples(num_samples);
  double inv_sample_rate = 1. / (double)sample_rate;
  for (uint64_t i = 0; i < num_samples; i++) {
    double value = sin(2 * M_PI * i * frequency * inv_sample_rate);
    samples[0][i] = max_amplitude * value;
    samples[1][i] = max_amplitude * value;
  }

  write_wav("out.wav", sample_rate, samples);
}

