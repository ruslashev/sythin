#include "lang.hh"
#include <algorithm>

std::string type_to_string(const type_t *const type) {
  if (type == nullptr)
    return "(null)";
  switch (type->kind) {
    case type_k::number:
      return "number";
    case type_k::lambda: {
      std::string type_str = "lambda: "
        , takes_str = type_to_string(type->lambda.takes)
        , returns_str = type_to_string(type->lambda.returns);
      if (type->lambda.takes != nullptr
          && type->lambda.takes->kind == type_k::lambda)
        type_str += "(" + takes_str + ")";
      else
        type_str += takes_str;
      type_str += " -> ";
      if (type->lambda.returns != nullptr
          && type->lambda.returns->kind == type_k::lambda)
        type_str += "(" + returns_str + ")";
      else
        type_str += returns_str;
      return type_str;
    } case type_k::builtin:
      return "builtin";
    default:
      return "unhandled";
  }
}

std::string builtin_kind_to_string(builtin_k kind) {
  switch (kind) {
    case builtin_k::sin:    return "sin";
    case builtin_k::exp:    return "exp";
    case builtin_k::inv:    return "inv";
    case builtin_k::plus:   return "plus";
    case builtin_k::minus:  return "minus";
    case builtin_k::mult:   return "mult";
    case builtin_k::divide: return "divide";
    case builtin_k::ceq:    return "ceq";
    case builtin_k::cneq:   return "cneq";
    case builtin_k::clt:    return "clt";
    case builtin_k::clteq:  return "clteq";
    case builtin_k::cgt:    return "cgt";
    case builtin_k::cgteq:  return "cgteq";
    case builtin_k::mod:    return "mod";
    case builtin_k::abs:    return "abs";
    default:                return "unhandled";
  }
}

builtin_t::~builtin_t() {
  switch (kind) {
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
    case builtin_k::abs:
      if (binary_op.x)
        delete binary_op.x;
      break;
    default:
      break;
  }
}

value_t::~value_t() {
  switch (type.kind) {
    case type_k::lambda:
      delete lambda.arg;
      delete lambda.body;
      break;
    case type_k::builtin:
      delete builtin;
      break;
    default:
      break;
  }
}

void value_t::pretty_print() const {
  switch (type.kind) {
    case type_k::number:
      printf("%4.2f", number);
      break;
    case type_k::lambda:
      printf("(\\ %s . ", lambda.arg->c_str());
      lambda.body->pretty_print();
      printf(")");
      break;
    case type_k::builtin:
      printf("%s", builtin_kind_to_string(builtin->kind).c_str());
      break;
    default:
      printf("unhandled");
      break;
  }
}

std::string term_kind_to_string(term_k kind) {
  switch (kind) {
    case term_k::definition:  return "definition";
    case term_k::application: return "application";
    case term_k::identifier:  return "identifier";
    case term_k::case_of:     return "case of";
    case term_k::if_else:     return "if else";
    case term_k::value:       return "value";
    default:                  return "unhandled";
  }
}

term_t::~term_t() {
  switch (kind) {
    case term_k::program:
      for (const term_t *const term : *program.terms)
        delete term;
      delete program.terms;
      break;
    case term_k::definition:
      delete definition.name;
      delete definition.body;
      break;
    case term_k::application:
      delete application.lambda;
      delete application.parameter;
      break;
    case term_k::identifier:
      delete identifier.name;
      break;
    case term_k::case_of:
      delete case_of.value;
      for (const case_statement &statement : *case_of.statements) {
        delete statement.value;
        delete statement.result;
      }
      delete case_of.statements;
      break;
    case term_k::if_else:
      delete if_else.condition;
      delete if_else.then_expr;
      delete if_else.else_expr;
      break;
    case term_k::value:
      delete value;
      break;
    default:
      break;
  }
  if (scope != nullptr) {
    delete scope;
    scope = nullptr;
  }
}

bool term_t::lookup(const std::string &identifier, value_t *&value) const {
  const term_t *it = this;
  while (it != nullptr) {
    if (it->scope != nullptr) {
      auto value_it = it->scope->find(identifier);
      if (value_it != it->scope->end()) {
        value = value_it->second;
        return true;
      }
    }
    it = it->parent;
  }
  return false;
}

void term_t::pretty_print() const {
  switch (kind) {
    case term_k::program:
      for (const term_t *const term : *program.terms) {
        term->pretty_print();
        printf("\n");
      }
      break;
    case term_k::definition:
      printf("%s = ", definition.name->c_str());
      definition.body->pretty_print();
      break;
    case term_k::application:
      printf("(");
      application.lambda->pretty_print();
      printf(" ");
      application.parameter->pretty_print();
      printf(")");
      break;
    case term_k::identifier:
      printf("%s", identifier.name->c_str());
      break;
    case term_k::case_of:
      printf("case ");
      case_of.value->pretty_print();
      printf(" of ");
      for (size_t i = 0; i < case_of.statements->size(); ++i) {
        const case_statement &statement = case_of.statements->at(i);
        if (statement.value)
          statement.value->pretty_print();
        else
          printf("_");
        printf(" -> ");
        statement.result->pretty_print();
        if (i != case_of.statements->size() - 1)
          printf(", ");
      }
      break;
    case term_k::if_else:
      printf("if ");
      if_else.condition->pretty_print();
      printf(" then ");
      if_else.then_expr->pretty_print();
      printf(" else ");
      if_else.else_expr->pretty_print();
      printf(" end");
      break;
    case term_k::value:
      value->pretty_print();
    default:
      break;
  }
}

std::string message_kind_to_string(message_k kind) {
  switch (kind) {
    case message_k::warning: return "warning";
    case message_k::error:   return "error";
    default:                 return "unhandled";
  }
}

bool messages_contain_no_errors(const std::vector<message_t> &messages) {
  for (const message_t &message : messages)
    if (message.kind == message_k::error)
      return false;
  return true;
}

std::vector<std::string> get_evaluatable_top_level_functions(const term_t
    *const term) {
  std::vector<std::string> fs;
  for (const term_t *term : *term->program.terms) {
    if (term->kind != term_k::definition)
      continue;
    term_t *main_lam = term->definition.body;
    if (main_lam->kind != term_k::value)
      continue;
    if (main_lam->value->type.kind != type_k::lambda)
      continue;
    value_t *lam_freq = main_lam->value;
    term_t *lam_time_term = lam_freq->lambda.body;
    if (lam_time_term->kind != term_k::value)
      continue;
    if (lam_time_term->value->type.kind != type_k::lambda)
      continue;
    std::string def = *term->definition.name;
    if (std::find(fs.begin(), fs.end(), def) == fs.end())
      fs.push_back(def);
  }
  return fs;
}

void validate_top_level_functions(const term_t *const term
    , std::vector<message_t> *messages) {
  if (term->kind != term_k::program) {
    messages->push_back({ message_k::error, "program parsing error" });
    return;
  }

  std::map<std::string, int> function_occurence_counter;
  for (const term_t *term : *term->program.terms) {
    if (term->kind != term_k::definition) {
      std::string message_content = "top-level term of type <"
        + term_kind_to_string(term->kind) + "> has no effect";
      messages->push_back({ message_k::warning, message_content });
      continue;
    }
    // remember that first call to operator[] initializes the counter with zero
    ++function_occurence_counter[*term->definition.name];
  }

  for (auto &occ_pair : function_occurence_counter)
    if (occ_pair.second > 1)
      messages->push_back({ message_k::error
          , "duplicate function \"" + occ_pair.first + "\"" });

  // if (function_occurence_counter.find("main")
  //     == function_occurence_counter.end())
  //   messages->push_back({ message_k::error, "no main function" });
}

builtin_t* builtin_sin() {
  builtin_t *b = new builtin_t;
  b->kind = builtin_k::sin;
  return b;
}

builtin_t* builtin_exp() {
  builtin_t *b = new builtin_t;
  b->kind = builtin_k::exp;
  return b;
}

builtin_t* builtin_inv() {
  builtin_t *b = new builtin_t;
  b->kind = builtin_k::inv;
  return b;
}

builtin_t* builtin_plus() {
  builtin_t *b = new builtin_t;
  b->kind = builtin_k::plus;
  b->binary_op.x = nullptr;
  return b;
}

builtin_t* builtin_minus() {
  builtin_t *b = new builtin_t;
  b->kind = builtin_k::minus;
  b->binary_op.x = nullptr;
  return b;
}

builtin_t* builtin_mult() {
  builtin_t *b = new builtin_t;
  b->kind = builtin_k::mult;
  b->binary_op.x = nullptr;
  return b;
}

builtin_t* builtin_divide() {
  builtin_t *b = new builtin_t;
  b->kind = builtin_k::divide;
  b->binary_op.x = nullptr;
  return b;
}

builtin_t* builtin_ceq() {
  builtin_t *b = new builtin_t;
  b->kind = builtin_k::ceq;
  b->binary_op.x = nullptr;
  return b;
}

builtin_t* builtin_cneq() {
  builtin_t *b = new builtin_t;
  b->kind = builtin_k::cneq;
  b->binary_op.x = nullptr;
  return b;
}

builtin_t* builtin_clt() {
  builtin_t *b = new builtin_t;
  b->kind = builtin_k::clt;
  b->binary_op.x = nullptr;
  return b;
}

builtin_t* builtin_clteq() {
  builtin_t *b = new builtin_t;
  b->kind = builtin_k::clteq;
  b->binary_op.x = nullptr;
  return b;
}

builtin_t* builtin_cgt() {
  builtin_t *b = new builtin_t;
  b->kind = builtin_k::cgt;
  b->binary_op.x = nullptr;
  return b;
}

builtin_t* builtin_cgteq() {
  builtin_t *b = new builtin_t;
  b->kind = builtin_k::cgteq;
  b->binary_op.x = nullptr;
  return b;
}

builtin_t* builtin_mod() {
  builtin_t *b = new builtin_t;
  b->kind = builtin_k::mod;
  b->binary_op.x = nullptr;
  return b;
}

builtin_t* builtin_abs() {
  builtin_t *b = new builtin_t;
  b->kind = builtin_k::abs;
  b->binary_op.x = nullptr;
  return b;
}

value_t* value_number(double number) {
  value_t *value = new value_t;
  value->type.kind = type_k::number;
  value->number = number;
  return value;
}

value_t* value_lambda(const std::string &arg, term_t *body) {
  value_t *value = new value_t;
  value->type.kind = type_k::lambda;
  // value->type.lambda.* to be filled in
  value->type.lambda.takes = nullptr;
  value->type.lambda.returns = nullptr;
  value->lambda.arg = new std::string(arg);
  value->lambda.body = body;
  return value;
}

value_t* value_builtin(builtin_t *builtin) {
  value_t *value = new value_t;
  value->type.kind = type_k::builtin;
  value->builtin = builtin;
  return value;
}

term_t* term_program(std::vector<term_t*> *terms) {
  term_t *t = new term_t;
  t->kind = term_k::program;
  t->program.terms = terms;
  for (term_t *term : *t->program.terms)
    term->parent = t;
  t->parent = nullptr;
  t->scope = nullptr;
  return t;
}

term_t* term_definition(const std::string &name, term_t *body) {
  term_t *t = new term_t;
  t->kind = term_k::definition;
  t->definition.name = new std::string(name);
  t->definition.body = body;
  t->definition.body->parent = t;
  t->parent = nullptr;
  t->scope = nullptr;
  return t;
}

term_t* term_application(term_t *lambda, term_t *parameter) {
  term_t *t = new term_t;
  t->kind = term_k::application;
  t->application.lambda = lambda;
  t->application.parameter = parameter;
  t->application.lambda->parent = t;
  t->application.parameter->parent = t;
  t->parent = nullptr;
  t->scope = nullptr;
  return t;
}

term_t* term_identifier(const std::string &name) {
  term_t *t = new term_t;
  t->kind = term_k::identifier;
  t->identifier.name = new std::string(name);
  t->parent = nullptr;
  t->scope = nullptr;
  return t;
}

term_t* term_case_of(term_t *value
    , std::vector<term_t::case_statement> *statements) {
  term_t *t = new term_t;
  t->kind = term_k::case_of;
  t->case_of.value = value;
  t->case_of.value->parent = t;
  t->case_of.statements = statements;
  for (const term_t::case_statement &statement : *t->case_of.statements) {
    if (statement.value)
      statement.value->parent = t;
    statement.result->parent = t;
  }
  t->parent = nullptr;
  t->scope = nullptr;
  return t;
}

term_t* term_if_else(term_t *condition, term_t *then_expr, term_t *else_expr) {
  term_t *t = new term_t;
  t->kind = term_k::if_else;
  t->if_else.condition = condition;
  t->if_else.then_expr = then_expr;
  t->if_else.else_expr = else_expr;
  t->if_else.condition->parent = t;
  t->if_else.then_expr->parent = t;
  t->if_else.else_expr->parent = t;
  t->parent = nullptr;
  t->scope = nullptr;
  return t;
}

term_t* term_value(value_t *value) {
  term_t *t = new term_t;
  t->kind = term_k::value;
  t->value = value;
  if (value->type.kind == type_k::lambda)
    t->value->lambda.body->parent = t;
  t->parent = nullptr;
  t->scope = nullptr;
  return t;
}

