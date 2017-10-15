#pragma once

#include "lang.hh"
#include <string>

std::string read_file(const std::string &filename);

const int TK_EOF = 0;

struct token_t {
  int kind; // enumeration values used from generated file
  int line, column;
  union {
    double number;
    std::string *identifier;
  };
  ~token_t();
  void pretty_print();
};

token_t* token_primitive(int line, int column, int kind);
token_t* token_number(int line, int column, double number);
token_t* token_identifier(int line, int column, std::string identifier);

class lexer_t {
  std::string _source;
  char _last_char;
  size_t _source_offset;
  int _line, _column;

  void _next_char();
  bool _is_newline(char x);
  bool _is_nonnl_whitespace(char x);
  bool _is_alpha(char x);
  bool _is_digit(char x);
  double _lex_number_decimal();
  double _lex_number_fraction();
  bool _try_lex_number_exponent(double *exponent);
public:
  lexer_t(const std::string &source);
  token_t* next_token();
};

term_t* parse_string(const std::string &source);

