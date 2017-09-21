#include "lang.hh"

std::string term_kind_to_string(term_k kind) {
  switch (kind) {
    case term_k::function:    return "function";
    case term_k::application: return "application";
    case term_k::variable:    return "variable";
    case term_k::number:      return "number";
    default:                  return "unhandled";
  }
}

std::string message_kind_to_string(message_k kind) {
  switch (kind) {
    case message_k::warning: return "warning";
    case message_k::error:   return "error";
    default:                 return "unhandled";
  }
}

bool program_t::validate_top_level_functions(std::vector<message_t> *messages)
  const {
  bool no_errors = true;
  std::map<std::string, int> function_occurence_counter;
  for (const term_t &term : terms) {
    if (term.type != term_k::function) {
      std::string message_content = "top-level term of type <"
        + term_kind_to_string(term.type) + "> has no effect";
      messages->push_back({ message_k::warning, message_content });
      continue;
    }
    if (*term.function.name == "main" && term.function.args->size() != 2) {
      no_errors = false;
      std::string message_content = "main function must take 2 arguments "
        "(frequency and time): expected 2, got "
        + std::to_string(term.function.args->size());
      messages->push_back({ message_k::error, message_content });
    }
    // remember that first call to operator[] initializes the counter with zero
    ++function_occurence_counter[*term.function.name];
  }

  for (auto &occ_pair : function_occurence_counter)
    if (occ_pair.second > 1) {
      no_errors = false;
      std::string message_content = "duplicate function \"" + occ_pair.first
        + "\"";
      messages->push_back({ message_k::error, message_content });
    }

  auto main_occ_it = function_occurence_counter.find("main");
  if (main_occ_it == function_occurence_counter.end()) {
    no_errors = false;
    std::string message_content = "no main function";
    messages->push_back({ message_k::error, message_content });
  }

  return no_errors;
}

bool scope_t::lookup(const std::string &variable, double *value) {
  // note that traversal is purposedfully in reverse order so that variables can
  // be overriden in deeper scopes (like in all sane languages)
  for (int i = stack.size() - 1; i >= 0; --i) {
    const std::map<std::string, double> &variables = stack[i];
    auto value_it = variables.find(variable);
    if (value_it != variables.end()) {
      *value = value_it->second;
      return true;
    }
  }
  return false;
}

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

