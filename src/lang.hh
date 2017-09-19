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
  } data;
};

term_t term_function(std::string name, std::vector<std::string> args
    , term_t body) {
  term_t t;
  t.type = term_k::function;
  t.data.function.name = new std::string(name);
  t.data.function.args = new std::vector<std::string>(args);
  t.data.function.body = new term_t(body);
  return t;
}

term_t term_application(std::string name, std::vector<term_t> parameters) {
  term_t t;
  t.type = term_k::application;
  t.data.application.name = new std::string(name);
  t.data.application.parameters = new std::vector<term_t>(parameters);
  return t;
}

term_t term_variable(std::string name) {
  term_t t;
  t.type = term_k::variable;
  t.data.variable.name = new std::string(name);
  return t;
}

term_t term_number(double value) {
  term_t t;
  t.type = term_k::number;
  t.data.number.value = value;
  return t;
}

// func f t = sin (2 * pi * f * t)
term_t func = term_function("func", std::vector<std::string>{ "f", "t" },
    term_application("mult", std::vector<term_t>{
        term_number(2),
        term_variable("pi"),
        term_variable("f"),
        term_variable("t")
      }
    )
  );

