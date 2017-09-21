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

struct program_t {
  std::vector<term_t> terms;
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
std::string term_type_to_string(const term_t &term);

