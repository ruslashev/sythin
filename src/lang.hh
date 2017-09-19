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

