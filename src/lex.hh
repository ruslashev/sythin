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

class lexer_t {
  std::string _source, _filename;
  char _last_char;
  size_t _source_offset;
  int _next_line, _next_column, _line, _column;
  std::vector<token_t*> _allocated_tokens;

  void _next_char();
  bool _is_newline(char x);
  bool _is_nonnl_whitespace(char x);
  bool _is_alpha(char x);
  bool _is_digit(char x);
  double _lex_number_decimal();
  double _lex_number_fraction();
  bool _try_lex_number_exponent(double *exponent);

  token_t* _token_primitive(int kind);
  token_t* _token_number(double number);
  token_t* _token_identifier(std::string identifier);
public:
  lexer_t(const std::string &source);
  ~lexer_t();
  token_t* next_token();
  std::string get_location();
};

struct yyunion_t {
  union {
    token_t *token;
    term_t *term;
    std::vector<term_t*> *term_list;
    std::vector<term_t::case_statement> *case_statement_list;
    term_t::case_statement *case_statement;
    value_t *value;
    builtin_t *builtin;
  };
};

term_t* lex_parse_string(const std::string &source);
int yylex(yyunion_t *yylval, lexer_t *lexer);
void yyerror(lexer_t *lexer, term_t **root, const char *error);

