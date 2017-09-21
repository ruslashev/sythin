#include <cmath>
#include "wav_writer.hh"
#include "lang.hh"
#include "utils.hh"

double evaluate_term(const term_t *const term, scope_t *scope) {
  switch (term->type) {
    case term_k::number:
      return term->number.value;
      break;
    case term_k::variable:
      double value;
      if (scope->lookup(*term->variable.name, &value))
        return value;
      die("unknown variable \"%s\"", term->variable.name->c_str());
      break;
    case term_k::application:
      if (*term->application.name == "sin") {
        assertf(term->application.parameters->size() == 1);
        return sin(evaluate_term(&((*term->application.parameters)[0]), scope));
      }
      if (*term->application.name == "mult") {
        double result = 1.;
        for (size_t i = 0; i < term->application.parameters->size(); ++i)
          result *= evaluate_term(&((*term->application.parameters)[i]), scope);
        return result;
      }
      die("unknown function \"%s\"", term->application.name->c_str());
      break;
    case term_k::function:
      die("wut");
  }
  return 0.; // shut up gcc
}

double evaluate_program(const program_t &program, double f, double t) {
  std::vector<message_t> messages;
  bool valid = program.validate_top_level_functions(&messages);
  for (const message_t &message : messages)
    printf("%s: %s\n", message.type == message_k::warning ? "warning"
        : message.type == message_k::error ? "error" : "message"
        , message.content.c_str());
  assertf(valid);

  scope_t scope;
  std::map<std::string, double> constants {
    { "pi", M_PI }
  };
  scope.stack.push_back(constants);
  for (const term_t &term : program.terms)
    if (*term.function.name != "main")
      continue;
    else {
      std::map<std::string, double> main_parameter_values;
      main_parameter_values[(*term.function.args)[0]] = f;
      main_parameter_values[(*term.function.args)[1]] = t;
      scope.stack.push_back(main_parameter_values);
      return evaluate_term(term.function.body, &scope);
    }
  die("things that shouldn't happen for 300");
}

int main() {
  // func f t = sin(mult(2, pi, f, t))
  program_t program = { std::vector<term_t>{
    term_function("main", std::vector<std::string>{ "f", "t" },
      term_application("sin", std::vector<term_t>{
        term_application("mult", std::vector<term_t>{
            term_number(2),
            term_variable("pi"),
            term_variable("f"),
            term_variable("t")
          })
        })
      )
    }
  };

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
    double value = evaluate_program(program, f, t);
    uint16_t w_value = amplitude * value;
    samples[0].push_back(w_value);
    samples[1].push_back(w_value);
  }

  write_wav("out.wav", sample_rate, samples);
}

