#include <cmath>
#include "wav_writer.hh"
#include "lang.hh"
#include "utils.hh"

value_t* evaluate_term(const term_t *const term, const program_t &program);

value_t* evaluate_application(const term_t *const term
    , const program_t &program) {
  value_t *lambda = nullptr
    , *parameter = evaluate_term(term->application.parameter, program);
  if (term->application.lambda->kind == term_k::identifier) {
    if (*term->application.lambda->identifier.name == "pred") {
      if (parameter->type.kind != type_k::number)
        die("pred/1: expected argument of type <number>, got <%s>"
            , type_to_string(&parameter->type).c_str());
      --parameter->number.value;
      return parameter;
    } else if (*term->application.lambda->identifier.name == "succ") {
      if (parameter->type.kind != type_k::number)
        die("succ/1: expected argument of type <number>, got <%s>"
            , type_to_string(&parameter->type).c_str());
      ++parameter->number.value;
      return parameter;
    } else if (*term->application.lambda->identifier.name == "sin") {
      if (parameter->type.kind != type_k::number)
        die("sin/1: expected argument of type <number>, got <%s>"
            , type_to_string(&parameter->type).c_str());
      parameter->number.value = sin(parameter->number.value);
      return parameter;
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
      die("unknown application identifier \"%s\""
          , term->application.lambda->identifier.name->c_str());
    lambda = evaluate_term(definition->definition.body, program);
  } else if (term->application.lambda->kind == term_k::application)
    lambda = evaluate_term(term->application.lambda, program);
  else
    die("unexpected application lambda kind <%s>"
        , term_kind_to_string(term->application.lambda->kind).c_str());

  if (lambda->type.kind != type_k::lambda)
    die("unexpected application lambda type <%s>, expected <lambda>"
        , type_to_string(&lambda->type).c_str());

  if (!lambda->lambda.body->scope)
    lambda->lambda.body->scope = new scope_t;
  (*lambda->lambda.body->scope)[*lambda->lambda.arg] = parameter;
  value_t *result = evaluate_term(lambda->lambda.body, program);
  return result;
}

value_t* evaluate_term(const term_t *const term, const program_t &program) {
  switch (term->kind) {
    case term_k::constant:
      return term->constant.value;
      break;
    case term_k::identifier:
      value_t *value;
      if (term->lookup(*term->identifier.name, value))
        return value;
      die("unknown identifier \"%s\"", term->identifier.name->c_str());
    case term_k::case_of: {
      value_t *value = evaluate_term(term->case_of.value, program);
      if (value->type.kind != type_k::number)
        die("anything but numbers are not supported in case statements yet");
      term_t *result = nullptr;
      for (const term_t::case_statement &statement : *term->case_of.statements)
        if (statement.value == nullptr) {
          result = statement.result;
          break;
        } else {
          value_t *statement_value = evaluate_term(statement.value, program);
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
      return evaluate_term(result, program);
      break;
    }
    case term_k::application:
      return evaluate_application(term, program);
    default:
      die("unexpected term kind <%s>", term_kind_to_string(term->kind).c_str());
  }
}

double evaluate_program(const program_t &program, double f, double t) {
  term_t *main_def = nullptr;
  for (term_t *term : program.terms) {
    if (term->kind != term_k::definition)
      continue;
    if (*term->definition.name != "main")
      continue;
    main_def = term;
    break;
  }

  main_def->scope = new scope_t;
  (*main_def->scope)["pi"] = value_number(M_PI);

  term_t *main_lam = main_def->definition.body;
  assertf(main_lam->kind == term_k::constant);
  assertf(main_lam->constant.value->type.kind == type_k::lambda);
  value_t *lam_freq = main_lam->constant.value;
  std::map<std::string, value_t*> main_parameter_value_freq;
  (*main_def->scope)[*lam_freq->lambda.arg] = value_number(f);

  term_t *lam_time_term = lam_freq->lambda.body;
  assertf(lam_time_term->kind == term_k::constant);
  assertf(lam_time_term->constant.value->type.kind == type_k::lambda);
  value_t *lam_time = lam_time_term->constant.value;
  std::map<std::string, value_t*> main_parameter_value_time;
  (*main_def->scope)[*lam_time->lambda.arg] = value_number(t);

  term_t *main_body = lam_time->lambda.body;
  value_t *program_result = evaluate_term(main_body, program);

  if (program_result->type.kind != type_k::number)
    die("program returned value of type <%s>, expected <number>"
        , type_to_string(&program_result->type).c_str());
  delete main_def->scope;
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
  // main = (\ f . (λ t . (sin ((mult ((mult f) t)) (double pi)))))
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
            term_identifier("sin"),
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
          )
        ))
      ))
    )
  }};

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

