#include <cmath>
#include "wav_writer.hh"

int main() {
  double max_amplitude = 32760;
  double sample_rate = 44100;
  double frequency = 261.626 /* C4 */, seconds = 2.5;
  int num_samples = sample_rate * seconds;
  samples_t samples = {
    std::vector<uint16_t>(num_samples), std::vector<uint16_t>(num_samples)
  };
  for (int n = 0; n < num_samples; n++) {
    double amplitude = (double)n / num_samples * max_amplitude;
    double value = sin((2 * M_PI * n * frequency) / sample_rate);
    samples[0][n] = amplitude  * value;
    samples[1][n] = (max_amplitude - amplitude) * value;
  }

  write_wav("out.wav", sample_rate, samples);
}

