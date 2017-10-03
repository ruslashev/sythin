#include <cmath>
#include "wav_writer.hh"
#include "lang.hh"
#include "utils.hh"

value_t* evaluate_term(const term_t *const term, const program_t &program
    , scope_t *scope);

value_t* evaluate_application(const term_t *const term, const program_t &program
    , scope_t *scope) {
  printf("app of type <%s>\n", term_kind_to_string(term->application.lambda->kind).c_str());
  value_t *lambda = nullptr;
  if (term->application.lambda->kind == term_k::identifier) {
    if (*term->application.lambda->identifier.name == "pred") {
      die("pred");
    }
    const term_t *definition = nullptr;
    for (const term_t *const tl_term : program.terms) {
      if (tl_term->kind != term_k::definition)
        continue;
      if (*tl_term->definition.name
          != *term->application.lambda->identifier.name)
        continue;
      definition = tl_term;
    }
    if (!definition)
      die("unknown function identifier \"%s\""
          , term->application.lambda->identifier.name->c_str());

    lambda = evaluate_term(definition->definition.body, program, scope);
  } else if (term->application.lambda->kind == term_k::application)
    lambda = evaluate_term(term->application.lambda, program, scope);

  assertf(lambda != nullptr);

  switch (lambda->type.kind) {
    case type_k::number:
      return lambda;
    case type_k::lambda: {
      value_t *parameter
        = evaluate_term(term->application.parameter, program, scope);
      std::map<std::string, value_t*> application_parameter;
      application_parameter[*lambda->lambda.arg] = parameter;
      scope->stack.push_back(application_parameter);
      value_t *result = evaluate_term(lambda->lambda.body
          , program, scope);
      scope->stack.pop_back();
      return result;
    }
    default:
      die("op");
  }
  die("dude");
}

value_t* evaluate_term(const term_t *const term, const program_t &program
    , scope_t *scope) {
  printf("eval <%s>: < ", term_kind_to_string(term->kind).c_str());
  term->pretty_print();
  printf(" >\n");
  switch (term->kind) {
    case term_k::constant:
      return term->constant.value;
      break;
    case term_k::identifier:
      value_t *value;
      if (scope->lookup(*term->identifier.name, value))
        return value;
      die("unknown identifier \"%s\"", term->identifier.name->c_str());
    case term_k::case_of: {
      value_t *value = evaluate_term(term->case_of.value, program, scope);
      if (value->type.kind != type_k::number)
        die("anything but numbers are not supported in case statements yet");
      term_t *result = nullptr;
      for (const term_t::case_statement &statement : *term->case_of.statements)
        if (statement.value == nullptr) {
          result = statement.result;
          break;
        } else {
          value_t *statement_value = evaluate_term(statement.value, program, scope);
          if (statement_value->type.kind != type_k::number)
            die("anything but numbers are not supported in case statements yet");
          long long int value_i = std::round(value->number.value)
            , statement_value_i = std::round(statement_value->number.value);
          if (value_i == statement_value_i) {
            result = statement.result;
            break;
          }
        }
      if (result == nullptr)
        die("no matching clause in case statement");
      return evaluate_term(result, program, scope);
      break;
    }
    case term_k::application:
      return evaluate_application(term, program, scope);
    default:
      die("unexpected term kind <%s>", term_kind_to_string(term->kind).c_str());
  }
}

double evaluate_program(const program_t &program, double f, double t) {
  const term_t *main_def = nullptr;
  for (const term_t *term : program.terms) {
    if (term->kind != term_k::definition)
      continue;
    if (*term->definition.name != "main")
      continue;
    main_def = term;
    break;
  }

  scope_t scope;
  std::map<std::string, value_t*> constants {
    { "pi", value_number(M_PI) }
  };
  scope.stack.push_back(constants);

  term_t *main_lam = main_def->definition.body;
  assertf(main_lam->kind == term_k::constant);
  assertf(main_lam->constant.value->type.kind == type_k::lambda);
  value_t *lam_freq = main_lam->constant.value;
  std::map<std::string, value_t*> main_parameter_value_freq;
  main_parameter_value_freq[*lam_freq->lambda.arg] = value_number(f);
  scope.stack.push_back(main_parameter_value_freq);

  term_t *lam_time_term = lam_freq->lambda.body;
  assertf(lam_time_term->kind == term_k::constant);
  assertf(lam_time_term->constant.value->type.kind == type_k::lambda);
  value_t *lam_time = lam_time_term->constant.value;
  std::map<std::string, value_t*> main_parameter_value_time;
  main_parameter_value_time[*lam_time->lambda.arg] = value_number(t);
  scope.stack.push_back(main_parameter_value_time);

  term_t *main_body = lam_time->lambda.body;
  value_t *program_result = evaluate_term(main_body, program, &scope);

  if (program_result->type.kind != type_k::number)
    die("program returned value of type <%s>, expected <number>"
        , type_to_string(&program_result->type).c_str());
  return program_result->number.value;
}

int main() {
  // add = (\x . (\y .
  // case y of
  //  0 -> x
  //  _ -> (succ (add x (pred y)))
  // ))
  // mult = (\x . (\y .
  // case y of
  //  0 -> x
  //  _ -> (add (mult x (pred y)) x)
  // ))
  // double = (\ a . (mult 2) a )
  // main = (\ f . (λ t . (mult ((mult f) t)) (double pi)))
  program_t program = { std::vector<term_t*>{
    term_definition("add",
      term_constant(value_lambda("x",
        term_constant(value_lambda("y",
          term_case_of(
            term_identifier("y"),
            new std::vector<term_t::case_statement> {
              { term_constant(value_number(0)), term_identifier("x") },
              { nullptr,
                term_application(
                  term_identifier("succ"),
                  term_application(
                    term_application(
                      term_identifier("add"),
                      term_identifier("x")
                    ),
                    term_application(
                      term_identifier("pred"),
                      term_identifier("y")
                    )
                  )
                )
              }
            }
          )
        ))
      ))
    ),

    term_definition("mult",
      term_constant(value_lambda("x",
        term_constant(value_lambda("y",
          term_case_of(
            term_identifier("y"),
            new std::vector<term_t::case_statement> {
              { term_constant(value_number(0)), term_identifier("x") },
              { nullptr,
                term_application(
                  term_application(
                    term_identifier("add"),
                    term_application(
                      term_application(
                        term_identifier("add"),
                        term_identifier("x")
                      ),
                      term_application(
                        term_identifier("pred"),
                        term_identifier("y")
                      )
                    )
                  ),
                  term_identifier("x")
                )
              }
            }
          )
        ))
      ))
    ),

    term_definition("double",
      term_constant(value_lambda("a",
        term_application(
          term_application(
            term_identifier("mult"),
            term_constant(value_number(2))
          ),
          term_identifier("a")
        )
      ))
    ),
    term_definition("main",
      term_constant(value_lambda("f",
        term_constant(value_lambda("t",
          term_application(
            term_application(
              term_identifier("mult"),
              term_application(
                term_application(
                  term_identifier("mult"),
                  term_identifier("f")
                ),
                term_identifier("t")
              )
            ),
            term_application(
              term_identifier("double"),
              term_identifier("pi")
            )
          )
        ))
      ))
    )
  }};
  puts("program:");
  program.pretty_print();
  puts("");

  std::vector<message_t> messages;
  program.validate_top_level_functions(&messages);
  for (const message_t &message : messages)
    printf("%s: %s\n", message_kind_to_string(message.kind).c_str()
        , message.content.c_str());
  assertf(messages_contain_no_errors(messages));

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

