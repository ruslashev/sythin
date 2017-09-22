#include <cmath>
#include "wav_writer.hh"
#include "lang.hh"
#include "utils.hh"

double evaluate_term(const term_t *const term, const program_t &program
    , scope_t *scope) {
  switch (term->type) {
    case term_k::number:
      return term->number.value;
      break;
    case term_k::identifier:
      double value;
      if (scope->lookup(*term->identifier.name, &value))
        return value;
      die("unknown identifier \"%s\"", term->identifier.name->c_str());
    case term_k::application:
      if (*term->application.name == "sin") {
        assertf(term->application.parameters->size() == 1);
        return sin(evaluate_term((*term->application.parameters)[0], program
              , scope));
      }
      if (*term->application.name == "mult") {
        double result = 1.;
        for (size_t i = 0; i < term->application.parameters->size(); ++i)
          result *= evaluate_term((*term->application.parameters)[i], program
              , scope);
        return result;
      }
      for (const term_t *tl_term : program.terms) {
        if (tl_term->type != term_k::function)
          continue;
        if (*tl_term->function.name != *term->application.name)
          continue;
        // it is guaranteed that function declarations are unique
        if (tl_term->function.args->size()
            != term->application.parameters->size())
          die("function application arguments mismatch: %s/%d =/= %s/%d"
              , tl_term->function.name->c_str()
              , (int)tl_term->function.args->size()
              , term->application.name->c_str()
              , (int)term->application.parameters->size());
        std::map<std::string, double> function_scope;
        for (size_t i = 0; i < tl_term->function.args->size(); ++i) {
          // strict evaluation
          const std::string &arg_name = (*tl_term->function.args)[i];
          double arg_value = evaluate_term(
              (*term->application.parameters)[i], program, scope);
          function_scope[arg_name] = arg_value;
        }
        scope->stack.push_back(function_scope);
        double result = evaluate_term(tl_term->function.body, program, scope);
        scope->stack.pop_back();
        return result;
      }
      die("unknown function \"%s\"", term->application.name->c_str());
    case term_k::function:
      die("wut");
  }
  return 0.; // shut up gcc
}

double evaluate_program(const program_t &program, double f, double t) {
  std::vector<message_t> messages;
  program.validate_top_level_functions(&messages);
  for (const message_t &message : messages)
    printf("%s: %s\n", message_kind_to_string(message.type).c_str()
        , message.content.c_str());
  assertf(messages_contain_no_errors(messages));

  scope_t scope;
  std::map<std::string, double> constants {
    { "pi", M_PI }
  };
  scope.stack.push_back(constants);
  for (const term_t *term : program.terms) {
    if (*term->function.name != "main")
      continue;
    std::map<std::string, double> main_parameter_values;
    main_parameter_values[(*term->function.args)[0]] = f;
    main_parameter_values[(*term->function.args)[1]] = t;
    scope.stack.push_back(main_parameter_values);
    return evaluate_term(term->function.body, program, &scope);
  }
  die("things that shouldn't happen for 300");
}

int main() {
  // double a = (λ x . mult(2) x) a
  // double a = mult(2, a)
  // func f t = sin(mult(f, t, double(pi))
  program_t program = { std::vector<term_t*>{
    term_function("double", std::vector<std::string>{ "a" },
      term_application("mult", std::vector<term_t*>{
        term_number(2),
        term_identifier("a"),
      })
    ),
    term_function("main", std::vector<std::string>{ "f", "t" },
      term_application("sin", std::vector<term_t*>{
        term_application("mult", std::vector<term_t*>{
          term_identifier("f"),
          term_identifier("t"),
          term_application("double", std::vector<term_t*>{
            term_identifier("pi")
          })
        })
      })
    )
  }};

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

