#pragma once

#include <deque>
#include <map>
#include <string>
#include <vector>

enum class type_k {
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
  // ~value_t() ?
  void pretty_print() const;
};

enum class term_k {
  function,
  application,
  identifier,
  constant
};

std::string term_kind_to_string(term_k kind);

struct term_t {
  term_k kind;
  union {
    struct {
      std::string *name;
      std::string *arg;
      term_t *body;
    } function;
    struct {
      term_t *lambda; // value (lambda) or identifier that aliases it
      term_t *parameter;
    } application;
    struct {
      std::string *name;
    } identifier;
    struct {
      value_t value;
    } constant; // should be named just 'value'?
  };
  ~term_t();
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

struct scope_t {
  std::deque<std::map<std::string, value_t>> stack; // can't iterate std::stack
  bool lookup(const std::string &identifier, value_t *value);
};

value_t value_number(double value);
value_t value_lambda(const std::string &arg, term_t *body);

term_t* term_function(const std::string &name, const std::string &arg
    , term_t *body);
term_t* term_application(term_t *lambda, term_t *parameter);
term_t* term_identifier(const std::string &name);
term_t* term_constant(value_t value);

