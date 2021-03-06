#pragma once

#include <map>
#include <string>
#include <vector>

enum class type_k {
  number,
  lambda,
  builtin
};

struct type_t {
  type_k kind;
  union {
    struct {
      type_t *takes, *returns;
    } lambda;
  };
};

std::string type_to_string(const type_t *const type);

struct term_t;

enum class builtin_k {
  sin,
  cos,
  exp,
  inv,
  plus,
  minus,
  mult,
  divide,
  ceq,
  cneq,
  clt,
  clteq,
  cgt,
  cgteq,
  mod,
  pow,
  abs,
  floor,
  round,
  ceil,
  sqrt
};

std::string builtin_kind_to_string(builtin_k kind);

struct builtin_t {
  builtin_k kind;
  union {
    struct {
      term_t *x; // can be null for partial application
    } binary_op;
  };
  ~builtin_t();
};

struct value_t {
  type_t type;
  union {
    double number;
    struct {
      std::string *arg;
      term_t *body;
    } lambda;
    builtin_t *builtin;
  };
  ~value_t();
  void pretty_print() const;
};

enum class term_k {
  program,
  definition,
  application,
  identifier,
  case_of,
  if_else,
  let_in,
  value
};

std::string term_kind_to_string(term_k kind);

typedef std::map<std::string, value_t*> scope_t;

struct term_t {
  struct case_statement {
    term_t *value; // can be null to mean "any"
    term_t *result;
  };

  term_k kind;
  union {
    struct {
      std::vector<term_t*> *terms;
    } program;
    struct {
      std::string *name;
      term_t *body;
    } definition;
    struct {
      term_t *lambda;
      term_t *parameter;
    } application;
    struct {
      std::string *name;
    } identifier;
    struct {
      term_t *value;
      std::vector<case_statement> *statements;
    } case_of;
    struct {
      term_t *condition;
      term_t *then_expr;
      term_t *else_expr;
    } if_else;
    struct {
      std::vector<term_t*> *definitions;
      term_t *body;
    } let_in;
    value_t *value;
  };

  term_t *parent;
  scope_t *scope;

  ~term_t();
  bool lookup(const std::string &identifier, value_t *&value) const;
  void pretty_print() const;
};

enum class message_k {
  warning,
  error
};

std::string message_kind_to_string(message_k kind);

struct message_t {
  message_k kind;
  std::string content;
};

bool messages_contain_no_errors(const std::vector<message_t> &messages);

std::vector<std::string> get_evaluatable_top_level_functions(const term_t *const term);
void validate_top_level_functions(const term_t *const term
    , std::vector<message_t> *messages);

builtin_t* builtin_unary(builtin_k kind);
builtin_t* builtin_binary(builtin_k kind);

value_t* value_number(double number);
value_t* value_lambda(const std::string &arg, term_t *body);
value_t* value_builtin(builtin_t *builtin);

term_t* term_program(std::vector<term_t*> *terms);
term_t* term_definition(const std::string &name, term_t *body);
term_t* term_application(term_t *lambda, term_t *parameter);
term_t* term_identifier(const std::string &name);
term_t* term_case_of(term_t *value
    , std::vector<term_t::case_statement> *statements);
term_t* term_if_else(term_t *condition, term_t *then_expr, term_t *else_expr);
term_t* term_let_in(std::vector<term_t*> *terms, term_t *body);
term_t* term_value(value_t *value);

