#include <cmath>
#include <fstream>
#include <vector>

template <typename T>
void write_int(std::ostream &stream, T value) {
  for (unsigned size = sizeof(T); size; --size, value >>= 8)
    stream.put(static_cast<char>(value & 0xFF));
}

int main() {
  std::ofstream f("out.wav", std::ios::binary);

  // http://soundfile.sapp.org/doc/WaveFormat
  // RIFF chunk
  uint32_t riff_id = 0x46464952; // "RIFF"
  uint32_t riff_size; // filled in later, 4 + (8 + SubChunk1Size) + (8 + SubChunk2Size)
  uint32_t riff_format = 0x45564157; // "WAVE"

  // fmt subchunk
  uint32_t fmt_id = 0x20746d66; // "fmt "
  uint32_t fmt_size = 16; // size of this 
  uint16_t fmt_audio_format = 1; // no compression
  uint16_t fmt_num_channels = 2;
  uint32_t fmt_sample_rate = 44100;
  uint16_t fmt_bits_per_sample = 16; // actually last in subchunk, reordered
  uint32_t fmt_byte_rate = fmt_sample_rate * fmt_num_channels
    * fmt_bits_per_sample / 8;
  uint16_t fmt_block_align = fmt_num_channels * fmt_bits_per_sample / 8;

  // data subchunk
  uint32_t data_id = 0x61746164; // "data"
  uint32_t data_size; // num_samples * num_channels * bits_per_sample / 8

  double max_amplitude = 32768;
  double hz = fmt_sample_rate;
  double frequency = 261.626 /* C4 */, seconds = 2.5;
  int num_samples = hz * seconds;  // total number of samples
  std::vector<std::vector<uint16_t>> samples = { // 2 channels
    std::vector<uint16_t>(num_samples), std::vector<uint16_t>(num_samples)
  };
  for (int n = 0; n < num_samples; n++) {
    double amplitude = (double)n / num_samples * max_amplitude;
    double value = sin((M_2_PI * n * frequency) / hz);
    samples[0][n] = amplitude  * value;
    samples[1][n] = (max_amplitude - amplitude) * value;
  }

  data_size = num_samples * fmt_num_channels * fmt_bits_per_sample / 8;
  riff_size = 4 + 8 + fmt_size + 8 + data_size;

  write_int(f, riff_id);
  write_int(f, riff_size);
  write_int(f, riff_format);
  write_int(f, fmt_id);
  write_int(f, fmt_size);
  write_int(f, fmt_audio_format);
  write_int(f, fmt_num_channels);
  write_int(f, fmt_sample_rate);
  write_int(f, fmt_byte_rate);
  write_int(f, fmt_block_align);
  write_int(f, fmt_bits_per_sample);
  write_int(f, data_id);
  write_int(f, data_size);
  for (size_t i = 0; i < samples[0].size(); ++i) {
    write_int(f, samples[0][i]);
    write_int(f, samples[1][i]);
  }
}

