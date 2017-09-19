#include <cmath>
#include "wav_writer.hh"
#include "lang.hh"
#include "utils.hh"

double evaluate_rec(const term_t * const term, double f, double t) {
  switch (term->type) {
    case term_k::number:
      return term->number.value;
      break;
    case term_k::variable:
      if (*term->variable.name == "f")
        return f;
      if (*term->variable.name == "t")
        return t;
      if (*term->variable.name == "pi")
        return M_PI;
      die("unknown variable \"%s\"", term->variable.name->c_str());
      break;
    case term_k::application:
      if (*term->application.name == "sin") {
        assertf(term->application.parameters->size() == 1);
        return sin(evaluate_rec(&((*term->application.parameters)[0]), f, t));
      }
      if (*term->application.name == "mult") {
        double result = 1.;
        for (size_t i = 0; i < term->application.parameters->size(); ++i)
          result *= evaluate_rec(&((*term->application.parameters)[i]), f, t);
        return result;
      }
      die("unknown function \"%s\"", term->application.name->c_str());
      break;
    case term_k::function:
      die("wut");
  }
  return 0.; // shut up gcc
}

double evaluate(const term_t &func, double f, double t) {
  // for now, assume program is not a list of functions, but a single function
  assertf(func.type == term_k::function);
  assertf(func.function.args->size() == 2);
  return evaluate_rec(func.function.body, f, t);
}

int main() {
  // func f t = sin (2 * pi * f * t)
  term_t func = term_function("func", std::vector<std::string>{ "f", "t" },
      term_application("sin", std::vector<term_t>{
        term_application("mult", std::vector<term_t>{
            term_number(2),
            term_variable("pi"),
            term_variable("f"),
            term_variable("t")
          })
        })
      );

  samples_t samples = { std::vector<uint16_t>(), std::vector<uint16_t>() };

  double amplitude = 32760;
  double sample_rate = 44100;
  double frequency = 261.626; // C4
  double seconds = 2.5;

  uint64_t num_samples = sample_rate * seconds;
  double inv_sample_rate = 1. / (double)sample_rate;
  for (uint64_t i = 0; i < num_samples; i++) {
    double f = frequency;
    double t = (double)i * inv_sample_rate;
    double value = evaluate(func, f, t);
    uint16_t w_value = amplitude * value;
    samples[0].push_back(w_value);
    samples[1].push_back(w_value);
  }

  write_wav("out.wav", sample_rate, samples);
}

