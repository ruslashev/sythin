#pragma once

#include <map>
#include <string>
#include <vector>

enum class type_k {
  undef, // may be unneeded
  number,
  lambda
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

struct value_t {
  type_t type;
  union {
    struct {
      double value;
    } number;
    struct {
      std::string *arg;
      term_t *body;
    } lambda;
  };
  value_t();
  ~value_t();
  void pretty_print() const;
};

enum class term_k {
  definition,
  application,
  identifier,
  case_of,
  constant // should be called 'value'
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
      value_t *value;
    } constant;
  };

  term_t *parent;
  scope_t *scope; // should be vector (or not?)

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

struct program_t {
  std::vector<term_t*> terms;
  ~program_t();
  void validate_top_level_functions(std::vector<message_t> *messages) const;
  void pretty_print();
};

value_t* value_number(double value);
value_t* value_lambda(const std::string &arg, term_t *body);

term_t* term_definition(const std::string &name, term_t *body);
term_t* term_application(term_t *lambda, term_t *parameter);
term_t* term_identifier(const std::string &name);
term_t* term_case_of(term_t *value
    , std::vector<term_t::case_statement> *statements);
term_t* term_constant(value_t *value);

