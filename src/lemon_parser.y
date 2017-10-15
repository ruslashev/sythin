%include {
#include "parse.hh"
#include <cassert>
}

%token_type { token_t* }
// %default_type { term_t* }
%extra_argument { term_t **root }

%type program { term_t* }
program ::= definition_list(def_list). { *root = term_program(def_list); puts("woash"); }

%type definition_list { std::vector<term_t*>* }
definition_list(out) ::= definition_list definition(def). { out->push_back(def); puts("new itemm"); }
definition_list(out) ::= definition(def). { out = new std::vector<term_t*>; out->push_back(def); puts("last deff"); }

%type definition { term_t* }
definition(out) ::= TK_IDENTIFIER(name) TK_EQUALS body(def_body) TK_EOS. { out = term_definition(*name->identifier, def_body); printf("ma nam defff\n"); }

%type body { term_t* }
body ::= application.
body(out) ::= TK_IDENTIFIER(ident). { out = term_identifier(*ident->identifier); printf("identt %s\n", ident->identifier->c_str()); }
body ::= case_of.
body(b) ::= value(v). { b = term_value(v); }

application ::= TK_LPAREN app_lambda app_parameter TK_RPAREN.

app_lambda ::= body.

app_parameter ::= body.

identifier ::= TK_IDENTIFIER.

case_of ::= TK_WORD_CASE case_value TK_WORD_OF case_statement_list.

case_value ::= body.

case_statement_list ::= case_statement_list case_statement.
case_statement_list ::= case_statement.

case_statement ::= TK_RARROW.

%type value { value_t* }
value(v) ::= TK_NUMBER(number_token). { v = value_number(number_token->number); printf("value numm %f\n", number_token->number); }
value ::= lambda.
value ::= builtin.

lambda ::= TK_LPAREN TK_LAMBDA lambda_arg TK_DOT lambda_body TK_RPAREN.

lambda_arg ::= identifier.

lambda_body ::= body.

builtin ::= TK_MULT.
builtin ::= TK_SIN.

