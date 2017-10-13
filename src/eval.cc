#include "eval.hh"
#include "utils.hh"
#include <cmath>

static value_t* evaluate_term(const term_t *const term
    , const term_t *const program, std::vector<value_t*> *garbage);

static value_t* evaluate_application(const term_t *const term
    , const term_t *const program, std::vector<value_t*> *garbage) {
  value_t *lambda = nullptr;
  switch (term->application.lambda->kind) {
    case term_k::identifier:
    case term_k::application:
    case term_k::value:
      lambda = evaluate_term(term->application.lambda, program, garbage);
      break;
    default:
      die("unexpected application lambda kind <%s>"
          , term_kind_to_string(term->application.lambda->kind).c_str());
  }

  switch (lambda->type.kind) {
    case type_k::builtin: {
      switch (lambda->builtin->kind) {
        case builtin_k::mult:
          if (lambda->builtin->mult.x == nullptr) {
            lambda->builtin->mult.x = term->application.parameter;
            return lambda;
          } else {
            value_t *stored_parameter
              = evaluate_term(lambda->builtin->mult.x, program, garbage);
            if (stored_parameter->type.kind != type_k::number)
              die("builtin mult/1: unexpected parameter of type <%s>, expected"
                  " <number>", type_to_string(&stored_parameter->type).c_str());
            value_t *applied_parameter
              = evaluate_term(term->application.parameter, program, garbage);
            if (applied_parameter->type.kind != type_k::number)
              die("builtin mult/1: applied to value of type <%s>, expected"
                  " <number>", type_to_string(&applied_parameter->type).c_str());
            lambda->builtin->mult.x = nullptr;
            // applied_parameter->number.value *= stored_parameter->number.value;
            // return applied_parameter;
            value_t *result = value_number(applied_parameter->number
                * stored_parameter->number);
            garbage->push_back(result);
            return result;
          }
        case builtin_k::sin: {
          value_t *applied_parameter
            = evaluate_term(term->application.parameter, program, garbage);
          if (applied_parameter->type.kind != type_k::number)
            die("builtin sin/1: unexpected parameter of type <%s>, expected"
                " <number>" , type_to_string(&applied_parameter->type).c_str());
          // applied_parameter->number.value = sin(applied_parameter->number.value);
          // return applied_parameter;
          value_t *result = value_number(sin(applied_parameter->number));
          garbage->push_back(result);
          return result;
        }
        default:
          die("unexpected builtin kind");
      }
      break;
    }
    case type_k::lambda: {
      value_t *parameter = evaluate_term(term->application.parameter, program
          , garbage);
      if (!lambda->lambda.body->scope)
        lambda->lambda.body->scope = new scope_t;
      (*lambda->lambda.body->scope)[*lambda->lambda.arg] = parameter;
      return evaluate_term(lambda->lambda.body, program, garbage);
      // maybe delete scope here and return later?
    }
    default:
      die("unexpected application lambda type <%s>"
          , type_to_string(&lambda->type).c_str());
  }
}

static value_t* evaluate_term(const term_t *const term
    , const term_t *const program, std::vector<value_t*> *garbage) {
  switch (term->kind) {
    case term_k::value:
      return term->value;
      break;
    case term_k::identifier: {
      // identifier can be in scope
      value_t *value;
      if (term->lookup(*term->identifier.name, value))
        return value;
      // or be a top-level definition
      const term_t *definition = nullptr;
      for (const term_t *const tl_term : *program->program.terms) {
        if (tl_term->kind != term_k::definition)
          continue;
        if (*tl_term->definition.name != *term->identifier.name)
          continue;
        definition = tl_term;
      }
      if (definition != nullptr)
        return evaluate_term(definition->definition.body, program, garbage);
      die("unknown identifier \"%s\"", term->identifier.name->c_str());
    }
    case term_k::case_of: {
      value_t *value = evaluate_term(term->case_of.value, program, garbage);
      if (value->type.kind != type_k::number)
        die("anything but numbers are not supported in case statements yet");
      term_t *result = nullptr;
      for (const term_t::case_statement &statement : *term->case_of.statements)
        if (statement.value == nullptr) {
          result = statement.result;
          break;
        } else {
          value_t *statement_value = evaluate_term(statement.value, program
              , garbage);
          if (statement_value->type.kind != type_k::number)
            die("anything but numbers are not supported in case statements yet");
          long long int value_i = std::round(value->number)
            , statement_value_i = std::round(statement_value->number);
          if (value_i == statement_value_i) {
            result = statement.result;
            break;
          }
        }
      if (result == nullptr)
        die("no matching clause in case statement");
      return evaluate_term(result, program, garbage);
      break;
    }
    case term_k::application:
      return evaluate_application(term, program, garbage);
    default:
      die("unexpected term kind <%s>", term_kind_to_string(term->kind).c_str());
  }
}

double evaluate_program(term_t *program, double f, double t) {
  term_t *main_def = nullptr;
  for (term_t *const term : *program->program.terms) {
    if (term->kind != term_k::definition)
      continue;
    if (*term->definition.name != "main")
      continue;
    main_def = term;
    break;
  }

  main_def->scope = new scope_t;

  value_t *value_freq = value_number(f), *value_time = value_number(t);

  std::vector<value_t*> garbage;
  garbage.push_back(value_freq);
  garbage.push_back(value_time);

  term_t *main_lam = main_def->definition.body;
  assertf(main_lam->kind == term_k::value);
  assertf(main_lam->value->type.kind == type_k::lambda);
  value_t *lam_freq = main_lam->value;
  std::map<std::string, value_t*> main_parameter_value_freq;
  (*main_def->scope)[*lam_freq->lambda.arg] = value_freq;

  term_t *lam_time_term = lam_freq->lambda.body;
  assertf(lam_time_term->kind == term_k::value);
  assertf(lam_time_term->value->type.kind == type_k::lambda);
  value_t *lam_time = lam_time_term->value;
  std::map<std::string, value_t*> main_parameter_value_time;
  (*main_def->scope)[*lam_time->lambda.arg] = value_time;

  term_t *main_body = lam_time->lambda.body;
  value_t *program_result = evaluate_term(main_body, program, &garbage);

  if (program_result->type.kind != type_k::number)
    die("program returned value of type <%s>, expected <number>"
        , type_to_string(&program_result->type).c_str());
  double result = program_result->number;

  for (const value_t *const value : garbage)
    delete value;
  delete main_def->scope;
  main_def->scope = nullptr;

  return result;
}

