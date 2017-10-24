#include "eval.hh"
#include "lang.hh"
#include "lex.hh"
#include "live.hh"
#include "utils.hh"
#include "wav_writer.hh"
#include <cmath>

int main(int argc, char **argv) {
  std::string source = read_file("test.sth");

  term_t *program = lex_parse_string(source);
  if (!program)
    exit(1);

  puts("parsed program:");
  program->pretty_print();

  std::vector<message_t> messages;
  validate_top_level_functions(program, &messages);
  for (const message_t &message : messages)
    printf("%s: %s\n", message_kind_to_string(message.kind).c_str()
        , message.content.c_str());
  if (!messages_contain_no_errors(messages))
    exit(1);

  if (1) {
    live(program, "main");
  } else {
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
  }
  delete program;
}

