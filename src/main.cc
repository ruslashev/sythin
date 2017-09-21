#include <cmath>
#include "wav_writer.hh"
#include "lang.hh"
#include "utils.hh"

// after every call to this function, `messages' should be checked for errors
double evaluate_term(const term_t *const term, const program_t &program
    , scope_t *scope, std::vector<message_t> *messages) {
  switch (term->type) {
    case term_k::number:
      return term->number.value;
    case term_k::variable:
      double value;
      if (scope->lookup(*term->variable.name, &value))
        return value;
      messages->push_back({ message_k::error, "unknown variable \""
          + *term->variable.name + "\"" });
      return 0;
    case term_k::application:
      if (*term->application.name == "sin") {
        if (term->application.parameters->size() != 1) {
          messages->push_back({ message_k::error, "function \"sin\" takes 1 "
              "argument, got "
              + std::to_string(term->application.parameters->size()) });
          return 0;
        }
        double parameter = evaluate_term(&((*term->application.parameters)[0])
            , program, scope, messages);
        if (!messages_contain_no_errors(*messages))
          return 0;
        return sin(parameter);
      }
      if (*term->application.name == "mult") {
        double result = 1.;
        for (size_t i = 0; i < term->application.parameters->size(); ++i) {
          result *= evaluate_term(&((*term->application.parameters)[i]), program
              , scope, messages);
          if (!messages_contain_no_errors(*messages))
            return 0;
        }
        return result;
      }
      for (const term_t &tl_term : program.terms) {
        if (tl_term.type != term_k::function)
          continue;
        if (*tl_term.function.name != *term->application.name)
          continue;
        // it is guaranteed that function declarations are unique
        if (tl_term.function.args->size()
            != term->application.parameters->size()) {
          messages->push_back({ message_k::error
              , "function application arguments mismatch: "
              + *tl_term.function.name + "/"
              + std::to_string(tl_term.function.args->size()) + " =/= "
              + *term->application.name
              + std::to_string(term->application.parameters->size()) });
          return 0;
        }
        std::map<std::string, double> function_scope;
        for (size_t i = 0; i < tl_term.function.args->size(); ++i) {
          // strict evaluation
          const std::string &arg_name = (*tl_term.function.args)[i];
          double arg_value = evaluate_term(
              &((*term->application.parameters)[i]), program, scope, messages);
          if (!messages_contain_no_errors(*messages))
            return 0;
          function_scope[arg_name] = arg_value;
        }
        scope->stack.push_back(function_scope);
        double result = evaluate_term(tl_term.function.body, program, scope
            , messages);
        if (!messages_contain_no_errors(*messages))
          return 0;
        scope->stack.pop_back();
        return result;
      }
      messages->push_back({ message_k::error, "unknown function \""
          + *term->application.name + "\"" });
      return 0;
    default:
      messages->push_back({ message_k::error, "unexpected term type" });
      return 0;
  }
}

double evaluate_program(const program_t &program, double f, double t) {
  std::vector<message_t> messages;
  program.validate_top_level_functions(&messages);
  for (const message_t &message : messages)
    printf("%s: %s\n", message_kind_to_string(message.type).c_str()
        , message.content.c_str());
  if (!messages_contain_no_errors(messages))
    exit(1);

  double result;
  messages.clear();
  scope_t scope;
  std::map<std::string, double> constants {
    { "pi", M_PI }
  };
  scope.stack.push_back(constants);
  for (const term_t &term : program.terms) {
    if (*term.function.name != "main")
      continue;
    std::map<std::string, double> main_parameter_values;
    main_parameter_values[(*term.function.args)[0]] = f;
    main_parameter_values[(*term.function.args)[1]] = t;
    scope.stack.push_back(main_parameter_values);
    result = evaluate_term(term.function.body, program, &scope, &messages);
  }

  if (!messages_contain_no_errors(messages))
    exit(1);
}

int main() {
  // double a = mult(2, a)
  // func f t = sin(mult(f, t, double(pi))
  program_t program = { std::vector<term_t>{
    term_function("double", std::vector<std::string>{ "a" },
      term_application("mult", std::vector<term_t>{
        term_number(2),
        term_variable("a"),
      })
    ),
    term_function("main", std::vector<std::string>{ "f", "t" },
      term_application("sin", std::vector<term_t>{
        term_application("mult", std::vector<term_t>{
          term_variable("f"),
          term_variable("t"),
          term_application("double", std::vector<term_t>{
            term_variable("pi")
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

