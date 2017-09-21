#pragma once

#include <string>
#include <vector>

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

term_t term_function(std::string name, std::vector<std::string> args
    , term_t body) {
  term_t t;
  t.type = term_k::function;
  t.function.name = new std::string(name);
  t.function.args = new std::vector<std::string>(args);
  t.function.body = new term_t(body);
  return t;
}

term_t term_application(std::string name, std::vector<term_t> parameters) {
  term_t t;
  t.type = term_k::application;
  t.application.name = new std::string(name);
  t.application.parameters = new std::vector<term_t>(parameters);
  return t;
}

term_t term_variable(std::string name) {
  term_t t;
  t.type = term_k::variable;
  t.variable.name = new std::string(name);
  return t;
}

term_t term_number(double value) {
  term_t t;
  t.type = term_k::number;
  t.number.value = value;
  return t;
}

std::string term_type_to_string(const term_t &term) {
  switch (term.type) {
    case term_k::function:    return "function";
    case term_k::application: return "application";
    case term_k::variable:    return "variable";
    case term_k::number:      return "number";
    default:                  return "unhandled";
  }
}

