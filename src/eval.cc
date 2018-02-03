#include "eval.hh"
#include "utils.hh"
#include <cmath>

static value_t* evaluate_term(term_t *term, const term_t *const program
    , std::vector<value_t*> *garbage);

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
      value_t *applied_parameter
        = evaluate_term(term->application.parameter, program, garbage);
      switch (lambda->builtin->kind) {
        case builtin_k::sin: {
          if (applied_parameter->type.kind != type_k::number)
            die("builtin sin/1: unexpected parameter of type <%s>, expected"
                " <number>" , type_to_string(&applied_parameter->type).c_str());
          value_t *result = value_number(sin(applied_parameter->number));
          garbage->push_back(result);
          return result;
        }
        case builtin_k::cos: {
          if (applied_parameter->type.kind != type_k::number)
            die("builtin cos/1: unexpected parameter of type <%s>, expected"
                " <number>" , type_to_string(&applied_parameter->type).c_str());
          value_t *result = value_number(cos(applied_parameter->number));
          garbage->push_back(result);
          return result;
        }
        case builtin_k::exp: {
          if (applied_parameter->type.kind != type_k::number)
            die("builtin exp/1: unexpected parameter of type <%s>, expected"
                " <number>" , type_to_string(&applied_parameter->type).c_str());
          value_t *result = value_number(exp(applied_parameter->number));
          garbage->push_back(result);
          return result;
        }
        case builtin_k::inv: {
          if (applied_parameter->type.kind != type_k::number)
            die("builtin inv/1: unexpected parameter of type <%s>, expected"
                " <number>" , type_to_string(&applied_parameter->type).c_str());
          value_t *result = value_number(-applied_parameter->number);
          garbage->push_back(result);
          return result;
        }
        case builtin_k::abs: {
          if (applied_parameter->type.kind != type_k::number)
            die("builtin abs/1: unexpected parameter of type <%s>, expected"
                " <number>" , type_to_string(&applied_parameter->type).c_str());
          value_t *result = value_number(std::fabs(applied_parameter->number));
          garbage->push_back(result);
          return result;
        }
        case builtin_k::floor: {
          if (applied_parameter->type.kind != type_k::number)
            die("builtin floor/1: unexpected parameter of type <%s>, expected"
                " <number>" , type_to_string(&applied_parameter->type).c_str());
          value_t *result = value_number(std::floor(applied_parameter->number));
          garbage->push_back(result);
          return result;
        }
        case builtin_k::round: {
          if (applied_parameter->type.kind != type_k::number)
            die("builtin round/1: unexpected parameter of type <%s>, expected"
                " <number>" , type_to_string(&applied_parameter->type).c_str());
          value_t *result = value_number(std::round(applied_parameter->number));
          garbage->push_back(result);
          return result;
        }
        case builtin_k::ceil: {
          if (applied_parameter->type.kind != type_k::number)
            die("builtin ceil/1: unexpected parameter of type <%s>, expected"
                " <number>" , type_to_string(&applied_parameter->type).c_str());
          value_t *result = value_number(std::ceil(applied_parameter->number));
          garbage->push_back(result);
          return result;
        }
        case builtin_k::sqrt: {
          if (applied_parameter->type.kind != type_k::number)
            die("builtin sqrt/1: unexpected parameter of type <%s>, expected"
                " <number>" , type_to_string(&applied_parameter->type).c_str());
          value_t *result = value_number(std::sqrt(applied_parameter->number));
          garbage->push_back(result);
          return result;
        }
        case builtin_k::plus:
        case builtin_k::minus:
        case builtin_k::mult:
        case builtin_k::divide:
        case builtin_k::ceq:
        case builtin_k::cneq:
        case builtin_k::clt:
        case builtin_k::clteq:
        case builtin_k::cgt:
        case builtin_k::cgteq:
        case builtin_k::mod:
        case builtin_k::pow:
          if (lambda->builtin->binary_op.x == nullptr) {
            lambda->builtin->binary_op.x = term->application.parameter;
            return lambda;
          } else {
            value_t *stored_parameter
              = evaluate_term(lambda->builtin->binary_op.x, program, garbage);
            if (stored_parameter->type.kind != type_k::number)
              die("builtin %s/1: unexpected parameter of type <%s>, expected"
                  " <number>"
                  , builtin_kind_to_string(lambda->builtin->kind).c_str()
                  , type_to_string(&stored_parameter->type).c_str());
            if (applied_parameter->type.kind != type_k::number)
              die("builtin %s/1: applied to value of type <%s>, expected"
                  " <number>"
                  , builtin_kind_to_string(lambda->builtin->kind).c_str()
                  , type_to_string(&applied_parameter->type).c_str());
            lambda->builtin->binary_op.x = nullptr;
            value_t *result;
            switch (lambda->builtin->kind) { // :born_to_think:
              case builtin_k::plus:
                result = value_number(stored_parameter->number
                    + applied_parameter->number);
                break;
              case builtin_k::minus:
                result = value_number(stored_parameter->number
                    - applied_parameter->number);
                break;
              case builtin_k::mult:
                result = value_number(stored_parameter->number
                    * applied_parameter->number);
                break;
              case builtin_k::divide:
                result = value_number(stored_parameter->number
                    / applied_parameter->number);
                break;
              case builtin_k::ceq:
                result = value_number((int64_t)stored_parameter->number
                    == (int64_t)applied_parameter->number);
                break;
              case builtin_k::cneq:
                result = value_number((int64_t)stored_parameter->number
                    != (int64_t)applied_parameter->number);
                break;
              case builtin_k::clt:
                result = value_number(stored_parameter->number
                    < applied_parameter->number);
                break;
              case builtin_k::clteq:
                result = value_number(stored_parameter->number
                    <= applied_parameter->number);
                break;
              case builtin_k::cgt:
                result = value_number(stored_parameter->number
                    > applied_parameter->number);
                break;
              case builtin_k::cgteq:
                result = value_number(stored_parameter->number
                    <= applied_parameter->number);
                break;
              case builtin_k::mod:
                result = value_number(std::fmod(stored_parameter->number
                      , applied_parameter->number));
                break;
              case builtin_k::pow:
                result = value_number(std::pow(stored_parameter->number
                      , applied_parameter->number));
                break;
              default: // silence warning
                break;
            }
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
      // maybe delete scope here and return later? probably not
    }
    default:
      die("unexpected application lambda type <%s>"
          , type_to_string(&lambda->type).c_str());
  }
}

static value_t* evaluate_term(term_t *term, const term_t *const program
    , std::vector<value_t*> *garbage) {
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
          if (std::llround(value->number) == std::llround(statement_value->number)) {
            result = statement.result;
            break;
          }
        }
      if (result == nullptr)
        die("no matching clause in case statement");
      return evaluate_term(result, program, garbage);
      break;
    }
    case term_k::if_else: {
      value_t *condition = evaluate_term(term->if_else.condition, program, garbage);
      if (condition->type.kind != type_k::number)
        die("anything but numbers are not supported in if statements yet");
      if ((int64_t)condition->number == 0)
        return evaluate_term(term->if_else.else_expr, program, garbage);
      else
        return evaluate_term(term->if_else.then_expr, program, garbage);
      break;
    }
    case term_k::let_in: {
      if (!term->scope)
        term->scope = new scope_t;
      for (const term_t *definition : *term->let_in.definitions) {
        std::string name = *definition->definition.name;
        term_t *body = definition->definition.body;
        (*term->scope)[name] = evaluate_term(body, program, garbage);
      }
      return evaluate_term(term->let_in.body, program, garbage);
      break;
    }
    case term_k::application:
      return evaluate_application(term, program, garbage);
    default:
      die("unexpected term kind <%s>", term_kind_to_string(term->kind).c_str());
  }
}

static void clear_scopes_rec(term_t *term) {
  switch (term->kind) {
    case term_k::program:
      for (term_t *tl_term : *term->program.terms)
        clear_scopes_rec(tl_term);
      break;
    case term_k::definition:
      clear_scopes_rec(term->definition.body);
      break;
    case term_k::application:
      clear_scopes_rec(term->application.lambda);
      clear_scopes_rec(term->application.parameter);
      break;
    case term_k::case_of:
      clear_scopes_rec(term->case_of.value);
      for (const term_t::case_statement &statement : *term->case_of.statements) {
        if (statement.value)
          clear_scopes_rec(statement.value);
        clear_scopes_rec(statement.result);
      }
      break;
    case term_k::if_else:
      clear_scopes_rec(term->if_else.condition);
      clear_scopes_rec(term->if_else.then_expr);
      clear_scopes_rec(term->if_else.else_expr);
      break;
    case term_k::let_in:
      for (term_t *definition : *term->let_in.definitions)
        clear_scopes_rec(definition);
      clear_scopes_rec(term->let_in.body);
      break;
    case term_k::value:
      switch (term->value->type.kind) {
        case type_k::lambda:
          clear_scopes_rec(term->value->lambda.body);
          break;
        default:
          break;
      }
      break;
    default:
      break;
  }
  if (term->scope != nullptr) {
    delete term->scope;
    term->scope = nullptr;
  }
}

double evaluate_definition(term_t *program, const std::string &name, double f
    , double t) {
  program->scope = new scope_t {
    { "pi", value_number(M_PI) }
  };

  term_t *def = nullptr;
  for (term_t *const term : *program->program.terms) {
    if (term->kind != term_k::definition)
      continue;
    if (*term->definition.name != name)
      continue;
    def = term;
    break;
  }
  if (def == nullptr)
    die("no definition \"%s\" found", name.c_str());

  def->scope = new scope_t;

  term_t *main_lam = def->definition.body;
  assertf(main_lam->kind == term_k::value);
  assertf(main_lam->value->type.kind == type_k::lambda);
  value_t *lam_freq = main_lam->value;
  std::map<std::string, value_t*> main_parameter_value_freq;
  (*def->scope)[*lam_freq->lambda.arg] = value_number(f);

  term_t *lam_time_term = lam_freq->lambda.body;
  assertf(lam_time_term->kind == term_k::value);
  assertf(lam_time_term->value->type.kind == type_k::lambda);
  value_t *lam_time = lam_time_term->value;
  std::map<std::string, value_t*> main_parameter_value_time;
  (*def->scope)[*lam_time->lambda.arg] = value_number(t);

  term_t *main_body = lam_time->lambda.body;

  std::vector<value_t*> garbage;
  for (const auto &program_scope_pair : *program->scope)
    garbage.push_back(program_scope_pair.second);
  for (const auto &main_scope_pair : *def->scope)
    garbage.push_back(main_scope_pair.second);

  value_t *program_result = evaluate_term(main_body, program, &garbage);

  if (program_result->type.kind != type_k::number)
    die("program returned value of type <%s>, expected <number>"
        , type_to_string(&program_result->type).c_str());
  double result = program_result->number;

  for (const value_t *const value : garbage)
    delete value;

  delete def->scope;
  def->scope = nullptr;
  delete program->scope;
  program->scope = nullptr;

  clear_scopes_rec(program);

  return result;
}

