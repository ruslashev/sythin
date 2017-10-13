#include "parse.hh"
#include "utils.hh"
#include <fstream>
#include <iostream>
#include <iterator>

std::string token_kind_to_string(token_k kind) {
  switch (kind) {
    case token_k::eof:         return "eof";
    case token_k::number:      return "number";
    case token_k::identifier:  return "identifier";
    case token_k::equals:      return "equals";
    case token_k::left_paren:  return "left_paren";
    case token_k::right_paren: return "right_paren";
    case token_k::lambda:      return "lambda";
    case token_k::dot:         return "dot";
    case token_k::word_case:   return "word_case";
    case token_k::word_of:     return "word_of";
    case token_k::right_arrow: return "right_arrow";
    default:                   return "unhandled";
  }
}

token_t::~token_t() {
  switch (kind) {
    case token_k::identifier:
      delete identifier;
      break;
    default:
      break;
  }
}

void token_t::pretty_print() {
  if (kind == token_k::number)
    printf("%s %4.2f\n", token_kind_to_string(kind).c_str(), number);
  else if (kind == token_k::identifier)
    printf("%s \"%s\"\n", token_kind_to_string(kind).c_str(), identifier->c_str());
  else
    printf("%s\n", token_kind_to_string(kind).c_str());
}

token_t* token_primitive(int line, int column, token_k kind) {
  token_t *t = new token_t;
  t->kind = kind;
  t->line = line;
  t->column = column;
  return t;
}

token_t* token_number(int line, int column, double number) {
  token_t *t = new token_t;
  t->kind = token_k::number;
  t->line = line;
  t->column = column;
  t->number = number;
  return t;
}

token_t* token_identifier(int line, int column, std::string identifier) {
  token_t *t = new token_t;
  t->kind = token_k::identifier;
  t->line = line;
  t->column = column;
  t->identifier = new std::string(identifier);
  return t;
}

void lexer_t::_next_char() {
  if (_source_offset <= _source.length()) {
    _last_char = _source[_source_offset++];
    if (_last_char == '\n') {
      ++_line;
      _column = 1;
    } else
      ++_column;
  } else
    _last_char = 0;
}

bool lexer_t::_is_alpha(char x) {
  return (x >= 'a' && x <= 'z')
    || (x >= 'A' && x <= 'Z');
}

bool lexer_t::_is_digit(char x) {
  return x >= '0' && x <= '9';
}

lexer_t::lexer_t()
  : _source("")
  , _last_char(' ') // hack to allow getting next token right away
  , _source_offset(0)
  , _line(1)
  , _column(1) {
}

void lexer_t::from_stdin() {
  std::cin >> std::noskipws;
  std::istream_iterator<char> it(std::cin), end;
  std::string results(it, end);
}

void lexer_t::from_file(const std::string &filename) {
  std::ifstream ifs(filename);
  if (!ifs)
    die("failed to open file \"%s\"", filename.c_str());
  std::string buffer { std::istreambuf_iterator<char>(ifs)
    , std::istreambuf_iterator<char>() };
  _source = std::move(buffer);
}

void lexer_t::from_string(const std::string &source) {
  _source = source;
}

/*
 * [ \n] skip;
 * [a-zA-Z][a-zA-Z0-9_]* identifier;
 */
token_t* lexer_t::next_token() {
  while (1) {
    if (_last_char == ' ' || _last_char == '\n')
      _next_char();
    if (_is_alpha(_last_char)) {
      std::string identifier = "";
      identifier += _last_char;
      _next_char();
      while (_is_alpha(_last_char) || _is_digit(_last_char) || _last_char == '_') {
        identifier += _last_char;
        _next_char();
      }
      if (identifier == "case")
        return token_primitive(_line, _column, token_k::word_case);
      if (identifier == "of")
        return token_primitive(_line, _column, token_k::word_of);
      return token_identifier(_line, _column, identifier);
    }
    if (_last_char == '=') {
      _next_char();
      return token_primitive(_line, _column, token_k::equals);
    }
    if (_last_char == '(') {
      _next_char();
      return token_primitive(_line, _column, token_k::left_paren);
    }
    if (_last_char == ')') {
      _next_char();
      return token_primitive(_line, _column, token_k::right_paren);
    }
    if (_last_char == '\\') {
      _next_char();
      return token_primitive(_line, _column, token_k::lambda);
    }
    if (_last_char == '.') {
      _next_char();
      return token_primitive(_line, _column, token_k::dot);
    }
    if (_last_char == 0)
      return token_primitive(_line, _column, token_k::eof);
    printf("unrecognized char '%c'\n", _last_char);
    _next_char();
  }
}

