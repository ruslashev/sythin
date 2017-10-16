%{
#include "parse.hh"
%}

%code requires {
#include "parse.hh"
}

// %glr-parser
// %language "C"
// %locations
%define api.pure full
%define api.value.type { token_t* }
// %define parse.lac true
// %define parse.error verbose
%output "bison_parser.cc"
%defines "bison_parser_tokens.hh"
%param { lexer_t *lexer }
%parse-param { term_t **root }

%token TK_IDENTIFIER TK_EQUALS TK_EOS TK_LPAREN TK_RPAREN TK_WORD_CASE
%token TK_WORD_OF TK_RARROW TK_NUMBER TK_LAMBDA TK_DOT TK_MULT TK_SIN TK_WORD_END

%%

program : %empty
        | definition_list;

definition_list : definition_list definition
                | definition;

definition : TK_EOS
           | TK_IDENTIFIER TK_EQUALS body TK_EOS;

body : other other
     | other;

other: identifier
     | value
     | case_of
     | TK_LPAREN body TK_RPAREN;

identifier : TK_IDENTIFIER;

case_of : TK_WORD_CASE body TK_WORD_OF case_statement_list TK_WORD_END;
case_statement_list : case_statement_list case_statement
                    | case_statement;
case_statement : body TK_RARROW body TK_EOS;

value : number
      | lambda
      | builtin;

number : TK_NUMBER;

lambda : TK_LPAREN TK_LAMBDA TK_IDENTIFIER TK_DOT body TK_RPAREN;

builtin : TK_MULT
        | TK_SIN;

