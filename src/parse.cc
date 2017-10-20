#include "parse.hh"
#include "bison_parser_tokens.hh"
#include "utils.hh"
#include <cmath>
#include <fstream>
#include <iostream>
#include <iterator>

std::string read_file(const std::string &filename) {
  std::ifstream ifs(filename);
  if (!ifs)
    die("failed to open file \"%s\"", filename.c_str());
  std::string buffer { std::istreambuf_iterator<char>(ifs)
    , std::istreambuf_iterator<char>() };
  return buffer;
}

std::string token_kind_to_string(int kind) {
  switch (kind) {
    case TK_EOF:        return "TK_EOF";
    case TK_IDENTIFIER: return "TK_IDENTIFIER";
    case TK_EQUALS:     return "TK_EQUALS";
    case TK_EOS:        return "TK_EOS";
    case TK_LPAREN:     return "TK_LPAREN";
    case TK_RPAREN:     return "TK_RPAREN";
    case TK_NUMBER:     return "TK_NUMBER";
    case TK_LAMBDA:     return "TK_LAMBDA";
    case TK_DOT:        return "TK_DOT";
    case TK_MULT:       return "TK_MULT";
    case TK_SIN:        return "TK_SIN";
    case TK_WORD_CASE:  return "TK_WORD_CASE";
    case TK_WORD_OF:    return "TK_WORD_OF";
    case TK_RARROW:     return "TK_RARROW";
    default:            return "unhandled";
  }
}

token_t::~token_t() {
  switch (kind) {
    case TK_IDENTIFIER:
      delete identifier;
      break;
    default:
      break;
  }
}

void token_t::pretty_print() {
  if (kind == TK_NUMBER)
    printf("%s %f\n", token_kind_to_string(kind).c_str(), number);
  else if (kind == TK_IDENTIFIER)
    printf("%s \"%s\"\n", token_kind_to_string(kind).c_str(), identifier->c_str());
  else
    printf("%s\n", token_kind_to_string(kind).c_str());
}

void lexer_t::_next_char() {
  if (_source_offset <= _source.length()) {
    _last_char = _source[_source_offset++];
    _line = _next_line;
    _column = _next_column;
    if (_last_char == '\n') {
      ++_next_line;
      _next_column = 1;
    } else
      ++_next_column;
  } else
    _last_char = 0;
}

bool lexer_t::_is_newline(char x) {
  return x == '\n' || x == '\r';
}

bool lexer_t::_is_nonnl_whitespace(char x) {
  return x == ' ' || x == '\t';
}

bool lexer_t::_is_alpha(char x) {
  return (x >= 'a' && x <= 'z')
    || (x >= 'A' && x <= 'Z');
}

bool lexer_t::_is_digit(char x) {
  return x >= '0' && x <= '9';
}

double lexer_t::_lex_number_decimal() {
  double decimal = 0;
  while (_is_digit(_last_char)) {
    decimal *= 10.;
    decimal += _last_char - '0';
    _next_char();
  }
  return decimal;
}

double lexer_t::_lex_number_fraction() {
  double fraction = 0, mult = 10.;
  while (_is_digit(_last_char)) {
    fraction += (_last_char - '0') / mult;
    mult *= 10.;
    _next_char();
  }
  return fraction;
}

bool lexer_t::_try_lex_number_exponent(double *exponent) {
  if (_last_char == 'e' || _last_char == 'E') {
    _next_char();
    int sign = 1;
    if (_last_char == '+')
      _next_char();
    else if (_last_char == '-') {
      sign = -1;
      _next_char();
    }
    if (_is_digit(_last_char)) {
      *exponent = sign * _lex_number_decimal();
      return true;
    } else
      die("unexpected character in number exponent at %d:%d", _line, _column);
  } else
    return false;
}

token_t* lexer_t::_token_primitive(int kind) {
  token_t *t = new token_t;
  _allocated_tokens.push_back(t);
  t->kind = kind;
  t->line = _line;
  t->column = _column;
  return t;
}

token_t* lexer_t::_token_number(double number) {
  token_t *t = new token_t;
  _allocated_tokens.push_back(t);
  t->kind = TK_NUMBER;
  t->line = _line;
  t->column = _column;
  t->number = number;
  return t;
}

token_t* lexer_t::_token_identifier(std::string identifier) {
  token_t *t = new token_t;
  _allocated_tokens.push_back(t);
  t->kind = TK_IDENTIFIER;
  t->line = _line;
  t->column = _column;
  t->identifier = new std::string(identifier);
  return t;
}

lexer_t::lexer_t(const std::string &source)
  : _source(source)
  , _filename("<string>")
  , _last_char(' ') // hack to allow getting next token right away
  , _source_offset(0)
  , _next_line(1)
  , _next_column(1)
  , _line(1)
  , _column(1) {
}

lexer_t::~lexer_t() {
  for (const token_t *const token : _allocated_tokens)
    delete token;
}

#if 0
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
#endif

/*
 * [ \n\r\t] skip;
 * [a-zA-Z][a-zA-Z0-9_]* identifier;
 * [+-]?(([0-9]+\.[0-9]+)|([0-9]+\.)|(\.[0-9]+)|([0-9]+))([eE][+-]?[0-9]+)?
 *   number;
 */
token_t* lexer_t::next_token() {
  while (1) {
    while (_is_nonnl_whitespace(_last_char) || _is_newline(_last_char))
      _next_char();
    if (_is_alpha(_last_char)) {
      std::string identifier = "";
      identifier += _last_char;
      _next_char();
      while (_is_alpha(_last_char) || _is_digit(_last_char) || _last_char == '_') {
        identifier += _last_char;
        _next_char();
      }
      if (identifier == "sin")
        return _token_primitive(TK_SIN);
      if (identifier == "case")
        return _token_primitive(TK_WORD_CASE);
      if (identifier == "of")
        return _token_primitive(TK_WORD_OF);
      if (identifier == "end")
        return _token_primitive(TK_WORD_END);
      return _token_identifier(identifier);
    }
    if (_last_char == '+' || _last_char == '-' || _is_digit(_last_char)
        || _last_char == '.') {
      int sign = 1;
      if (_last_char == '+')
        _next_char();
      else if (_last_char == '-') {
        _next_char();
        sign = -1;
      }
      if (_is_digit(_last_char)) {
        double decimal = _lex_number_decimal();
        if (_last_char == '.') {
          _next_char();
          if (_is_digit(_last_char)) {
            double fraction = _lex_number_fraction(), exponent;
            if (_try_lex_number_exponent(&exponent))
              return _token_number(sign * ((decimal + fraction)
                    * pow(10., exponent)));
            else
              return _token_number(sign * (decimal + fraction));
          } else {
            double exponent;
            if (_try_lex_number_exponent(&exponent))
              return _token_number(sign * decimal
                  * pow(10., exponent));
            else
              return _token_number(sign * decimal);
          }
        } else {
          double exponent;
          if (_try_lex_number_exponent(&exponent))
            return _token_number(sign * decimal
                * pow(10., exponent));
          else
            return _token_number(sign * decimal);
        }
      } else if (_last_char == '.') {
        _next_char();
        if (_is_digit(_last_char)) {
          double fraction = _lex_number_fraction(), exponent;
          if (_try_lex_number_exponent(&exponent))
            return _token_number(sign * fraction
                * pow(10., exponent));
          else
            return _token_number(sign * fraction);
        } else
          return _token_primitive(TK_DOT);
      } else if (_last_char == '>' && sign == -1) {
        _next_char();
        return _token_primitive(TK_RARROW);
      } else
        die("unexpected character in number at %d:%d", _line, _column);
    }
    if (_last_char == '*') {
      _next_char();
      return _token_primitive(TK_MULT);
    }
    if (_last_char == '=') {
      _next_char();
      return _token_primitive(TK_EQUALS);
    }
    if (_last_char == '(') {
      _next_char();
      return _token_primitive(TK_LPAREN);
    }
    if (_last_char == ')') {
      _next_char();
      return _token_primitive(TK_RPAREN);
    }
    if (_last_char == '\\') {
      _next_char();
      return _token_primitive(TK_LAMBDA);
    }
    if (_last_char == '#') {
      _next_char();
      while (!_is_newline(_last_char))
        _next_char();
      _next_char();
      continue;
    }
    if (_last_char == ',') {
      _next_char();
      return _token_primitive(TK_EOS);
    }
    if (_last_char == '_') {
      _next_char();
      return _token_primitive(TK_ANY);
    }
    if (_last_char == 0)
      return _token_primitive(TK_EOF);
    printf("unrecognized char '%c'\n", _last_char);
    _next_char();
  }
}

std::string lexer_t::get_location() {
  return _filename + ":" + std::to_string(_line) + ":" + std::to_string(_column);
}

term_t* parse_string(const std::string &source) {
  lexer_t lexer(source);
  term_t *root = nullptr;
  yyparse(&lexer, &root);
  return root;
}

int yylex(yyunion_t *yylval, lexer_t *lexer) {
  token_t *t = lexer->next_token();
  yylval->token = t;
  return t->kind;
}

void yyerror(lexer_t *lexer, term_t **root, const char *error) {
  printf("%s: %s\n", lexer->get_location().c_str(), error);
}

