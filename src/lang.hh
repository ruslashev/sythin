#pragma once

#include <deque>
#include <map>
#include <string>
#include <vector>

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
      std::vector<term_t*> *parameters;
    } application;
    struct {
      std::string *name;
    } variable;
    struct {
      double value;
    } number;
  };
  ~term_t();
};

enum class message_k {
  warning,
  error
};

std::string message_kind_to_string(message_k kind);

struct message_t {
  message_k type;
  std::string content;
};

bool messages_contain_no_errors(const std::vector<message_t> &messages);

struct program_t {
  std::vector<term_t*> terms;
  ~program_t();
  void validate_top_level_functions(std::vector<message_t> *messages) const;
};

struct scope_t {
  std::deque<std::map<std::string, double>> stack; // can't iterate std::stack
  bool lookup(const std::string &variable, double *value);
};

term_t* term_function(const std::string &name, std::vector<std::string> args
    , term_t *body);
term_t* term_application(const std::string &name
    , std::vector<term_t*> parameters);
term_t* term_variable(const std::string &name);
term_t* term_number(double value);

