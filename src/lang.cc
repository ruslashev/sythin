#include "lang.hh"

std::string type_to_string(const type_t *const type) {
  if (type == nullptr)
    return "(null)";
  switch (type->kind) {
    case type_k::undef:
      return "undef";
    case type_k::number:
      return "number";
    case type_k::lambda: {
      std::string type_str = "lambda: "
        , takes_str = type_to_string(type->lambda.takes)
        , returns_str = type_to_string(type->lambda.returns);
      if (type->lambda.takes != nullptr
          && type->lambda.takes->kind == type_k::lambda)
        type_str += "(" + takes_str + ")";
      else
        type_str += takes_str;
      type_str += " -> ";
      if (type->lambda.returns != nullptr
          && type->lambda.returns->kind == type_k::lambda)
        type_str += "(" + returns_str + ")";
      else
        type_str += returns_str;
      return type_str;
    }
    default:
      return "unhandled";
  }
}

value_t::value_t() {
  type.kind = type_k::undef;
}

value_t::~value_t() {
  switch (type.kind) {
    case type_k::lambda:
      delete lambda.arg;
      delete lambda.body;
      break;
    default:
      break;
  }
}

static void print_indent(int level) {
  for (int i = 0; i < level; ++i)
    printf(" ");
}

void value_t::pretty_print() const {
  switch (type.kind) {
    case type_k::number:
      printf("%4.2f", number.value);
      break;
    case type_k::lambda:
      printf("(\\ %s . ", lambda.arg->c_str());
      lambda.body->pretty_print();
      printf(")");
      break;
    default:
      printf("unhandled");
      break;
  }
}

std::string term_kind_to_string(term_k kind) {
  switch (kind) {
    case term_k::definition:  return "definition";
    case term_k::application: return "application";
    case term_k::identifier:  return "identifier";
    case term_k::case_of:     return "case of";
    case term_k::constant:    return "constant";
    default:                  return "unhandled";
  }
}

term_t::~term_t() {
  switch (kind) {
    case term_k::definition:
      delete definition.name;
      delete definition.body;
      break;
    case term_k::application:
      delete application.lambda;
      delete application.parameter;
      break;
    case term_k::identifier:
      delete identifier.name;
      break;
    case term_k::case_of:
      delete case_of.value;
      for (const case_statement &statement : *case_of.statements) {
        delete statement.value;
        delete statement.result;
      }
      delete case_of.statements;
      break;
    case term_k::constant:
      delete constant.value;
    default:
      break;
  }
}

void term_t::pretty_print() const {
  switch (kind) {
    case term_k::definition:
      printf("%s = ", definition.name->c_str());
      definition.body->pretty_print();
      break;
    case term_k::application:
      printf("(");
      application.lambda->pretty_print();
      printf(" ");
      application.parameter->pretty_print();
      printf(")");
      break;
    case term_k::identifier:
      printf("%s", identifier.name->c_str());
      break;
    case term_k::case_of:
      printf("case ");
      case_of.value->pretty_print();
      printf(" of\n");
      for (const case_statement &statement : *case_of.statements) {
        print_indent(1);
        if (statement.value)
          statement.value->pretty_print();
        else
          printf("_");
        printf(" -> ");
        statement.result->pretty_print();
        printf("\n");
      }
      break;
    case term_k::constant:
      constant.value->pretty_print();
    default:
      break;
  }
}

std::string message_kind_to_string(message_k kind) {
  switch (kind) {
    case message_k::warning: return "warning";
    case message_k::error:   return "error";
    default:                 return "unhandled";
  }
}

bool messages_contain_no_errors(const std::vector<message_t> &messages) {
  for (const message_t &message : messages)
    if (message.kind == message_k::error)
      return false;
  return true;
}

program_t::~program_t() {
  for (const term_t *term : terms)
    delete term;
}

void program_t::validate_top_level_functions(std::vector<message_t> *messages)
  const {
  std::map<std::string, int> function_occurence_counter;
  for (const term_t *term : terms) {
    if (term->kind != term_k::definition) {
      std::string message_content = "top-level term of type <"
        + term_kind_to_string(term->kind) + "> has no effect";
      messages->push_back({ message_k::warning, message_content });
      continue;
    }
    /*
    if (*term->function.name == "main" && term->function.body. != 2) {
      std::string message_content = "main function must take 2 arguments "
        "(frequency and time): expected 2, got "
        + std::to_string(term->function.args->size());
      messages->push_back({ message_k::error, message_content });
    }
    */
    // remember that first call to operator[] initializes the counter with zero
    ++function_occurence_counter[*term->definition.name];
  }

  for (auto &occ_pair : function_occurence_counter)
    if (occ_pair.second > 1) {
      std::string message_content = "duplicate function \"" + occ_pair.first
        + "\"";
      messages->push_back({ message_k::error, message_content });
    }

  auto main_occ_it = function_occurence_counter.find("main");
  if (main_occ_it == function_occurence_counter.end()) {
    std::string message_content = "no main function";
    messages->push_back({ message_k::error, message_content });
  }
}

void program_t::pretty_print() {
  for (const term_t *const term : terms) {
    term->pretty_print();
    printf("\n");
  }
}

bool scope_t::lookup(const std::string &identifier, value_t *&value) {
  // note that traversal is purposedfully in reverse order so that variables can
  // be overriden in deeper scopes (like in all sane languages)
  for (int i = stack.size() - 1; i >= 0; --i) {
    const std::map<std::string, value_t*> &identifiers = stack[i];
    auto value_it = identifiers.find(identifier);
    if (value_it != identifiers.end()) {
      value = value_it->second;
      return true;
    }
  }
  return false;
}

value_t* value_number(double number_value) {
  value_t* value = new value_t;
  value->type.kind = type_k::number;
  value->number.value = number_value;
  return value;
}

value_t* value_lambda(const std::string &arg, term_t *body) {
  value_t* value = new value_t;
  value->type.kind = type_k::lambda;
  // value->type.lambda.* to be filled in
  value->type.lambda.takes = nullptr;
  value->type.lambda.returns = nullptr;
  value->lambda.arg = new std::string(arg);
  value->lambda.body = body;
  return value;
}

term_t* term_definition(const std::string &name, term_t *body) {
  term_t *t = new term_t;
  t->kind = term_k::definition;
  t->definition.name = new std::string(name);
  t->definition.body = body;
  return t;
}

term_t* term_application(term_t *lambda, term_t *parameter) {
  term_t *t = new term_t;
  t->kind = term_k::application;
  t->application.lambda = lambda;
  t->application.parameter = parameter;
  return t;
}

term_t* term_identifier(const std::string &name) {
  term_t *t = new term_t;
  t->kind = term_k::identifier;
  t->identifier.name = new std::string(name);
  return t;
}

term_t* term_case_of(term_t *value
    , std::vector<term_t::case_statement> *statements) {
  term_t *t = new term_t;
  t->kind = term_k::case_of;
  t->case_of.value = value;
  t->case_of.statements = statements;
  return t;
}

term_t* term_constant(value_t *value) {
  term_t *t = new term_t;
  t->kind = term_k::constant;
  t->constant.value = value;
  return t;
}

