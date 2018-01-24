#include "eval.hh"
#include "lang.hh"
#include "lex.hh"
#include "live.hh"
#include "utils.hh"
#include "wav_writer.hh"
#include "../thirdparty/clipp/clipp.h"
#include <cmath>
#include <iostream>

int main(int argc, char **argv) {
  std::string filename = "", seq_filename = "";
  bool seq = false;

  auto cli = (clipp::value("source file name", filename),
      clipp::option("--seq", "-s").set(seq).doc("sequence mode")
      & clipp::value("sequence file", seq_filename));

  if (!clipp::parse(argc, argv, cli)) {
    std::cout << make_man_page(cli, argv[0]);
    exit(1);
  }

  if (seq) {
    exit(0);

    std::string source = read_file("test.sth");

    term_t *program = lex_parse_string(source);
    if (!program)
      exit(1);

    samples_t samples = { std::vector<uint16_t>(), std::vector<uint16_t>() };
    double amplitude = 32760, sample_rate = 44100, frequency = 261.626 // C4
      , seconds = 2.5;

    uint64_t num_samples = sample_rate * seconds;
    double seconds_per_sample = 1. / (double)sample_rate;
    for (uint64_t i = 0; i < num_samples; i++) {
      double f = frequency, t = (double)i * seconds_per_sample
        , value = evaluate_definition(program, "main", f, t);
      uint16_t w_value = std::round(amplitude * value);
      samples[0].push_back(w_value);
      samples[1].push_back(w_value);
    }

    write_wav("out.wav", sample_rate, samples);

    delete program;
  }

  live(filename);
}

