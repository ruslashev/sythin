#pragma once

#include <string>

enum class token_k {
  eof,
  number,
  identifier,
  equals,
  left_paren,
  right_paren,
  lambda,
  dot,
  word_case,
  word_of,
  right_arrow,
  plus,
  minus,
  multiply,
  divide
};

std::string token_kind_to_string(token_k kind);

struct token_t {
  token_k kind;
  int line, column;
  union {
    double number;
    std::string *identifier;
  };
  ~token_t();
  void pretty_print();
};

token_t* token_primitive(int line, int column, token_k kind);
token_t* token_number(int line, int column, double number);
token_t* token_identifier(int line, int column, std::string identifier);

class lexer_t {
  std::string _source;
  char _last_char;
  size_t _source_offset;
  int _line, _column;

  void _next_char();
  bool _is_whitespace(char x);
  bool _is_alpha(char x);
  bool _is_digit(char x);
public:
  lexer_t();
  void from_stdin();
  void from_file(const std::string &filename);
  void from_string(const std::string &source);
  token_t* next_token();
};

