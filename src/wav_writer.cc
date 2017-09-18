#include "wav_writer.hh"
#include <fstream>

// note that it writes different # of bytes depending on type of value
template <typename T> static void write_int(std::ostream &stream, T value) {
  for (unsigned size = sizeof(T); size; --size, value >>= 8)
    stream.put(static_cast<char>(value & 0xFF));
}

void write_wav(const char *filename, int sample_rate
    , samples_t samples) {
  size_t num_samples = samples[0].size();

  // http://soundfile.sapp.org/doc/WaveFormat
  // RIFF chunk
  uint32_t riff_id = 0x46464952; // "RIFF"
  uint32_t riff_format = 0x45564157; // "WAVE"

  // fmt subchunk (subchunk 1)
  uint32_t fmt_id = 0x20746d66; // "fmt "
  uint32_t fmt_size = 16; // size of this subchunk
  uint16_t fmt_audio_format = 1; // no compression
  uint16_t fmt_num_channels = 2;
  uint32_t fmt_sample_rate = 44100;
  uint16_t fmt_bits_per_sample = 16; // actually last in subchunk, reordered
  uint32_t fmt_byte_rate = fmt_sample_rate * fmt_num_channels
    * fmt_bits_per_sample / 8;
  uint16_t fmt_block_align = fmt_num_channels * fmt_bits_per_sample / 8;

  // data subchunk (subchunk 2)
  uint32_t data_id = 0x61746164; // "data"
  // num_samples * num_channels * bits_per_sample / 8
  uint32_t data_size = num_samples * fmt_num_channels * fmt_bits_per_sample / 8;

  // belongs to riff chunk, 4 + (8 + SubChunk1Size) + (8 + SubChunk2Size)
  uint32_t riff_size = 4 + 8 + fmt_size + 8 + data_size;

  std::ofstream f(filename, std::ios::binary);

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
  for (size_t i = 0; i < num_samples; ++i) {
    write_int(f, samples[0][i]);
    write_int(f, samples[1][i]);
  }
}

