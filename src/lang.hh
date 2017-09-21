#pragma once

#include <string>
#include <vector>
#include <map>

enum class term_k {
  function,
  application,
  variable,
  number
};

std::string term_kind_to_string(term_k kind);

struct term_t {
  term_k type;
  union {
    struct {
      std::string *name;
      std::vector<std::string> *args;
      term_t *body;
    } function;
    struct {
      std::string *name;
      std::vector<term_t> *parameters;
    } application;
    struct {
      std::string *name;
    } variable;
    struct {
      double value;
    } number;
  };
};

enum message_k {
  warning,
  error
};

std::string message_kind_to_string(message_k kind);

struct message_t {
  message_k type;
  std::string content;
};

struct program_t {
  std::vector<term_t> terms;
  bool validate_top_level_functions(std::vector<message_t> *messages) const;
};

struct scope_t {
  std::vector<std::map<std::string, double>> stack; // can't iterate std::stack
  bool lookup(const std::string &variable, double *value);
};

term_t term_function(std::string name, std::vector<std::string> args
    , term_t body);
term_t term_application(std::string name, std::vector<term_t> parameters);
term_t term_variable(std::string name);
term_t term_number(double value);

