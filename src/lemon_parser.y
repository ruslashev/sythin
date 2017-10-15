%include {
#include "parse.hh"
#include <cassert>
}

%token_type { token_t* }
%default_type { term_t* }
%extra_argument { term_t **root }

program ::= definition_list(def_list). { *root = term_program(def_list); }

%type definition_list { std::vector<term_t*>* }
definition_list(d) ::= definition_list definition(def). {
  d->push_back(def);
}
definition_list(d) ::= definition(def). {
  d = new std::vector<term_t*>;
  d->push_back(def);
}

definition(d) ::= TK_IDENTIFIER(name) TK_EQUALS body(def_body) TK_EOS. {
  d = term_definition(*name->identifier, def_body);
}

body(b) ::= application(a). { b = a; }
body(b) ::= identifier(i). { b = i; }
body ::= case_of.
body(b) ::= value(v). { b = term_value(v); }

application(a) ::= TK_LPAREN app_lambda(lambda) app_parameter(parameter) TK_RPAREN. {
  a = term_application(lambda, parameter);
}

app_lambda ::= body.

app_parameter ::= body.

identifier(i) ::= TK_IDENTIFIER(token). {
  i = term_identifier(*token->identifier);
}

case_of ::= TK_WORD_CASE case_value TK_WORD_OF case_statement_list.

case_value ::= body.

case_statement_list ::= case_statement_list case_statement.
case_statement_list ::= case_statement.

case_statement ::= TK_RARROW.

%type value { value_t* }
value(v) ::= number(n). { v = n; }
value(v) ::= lambda(l). { v = l; }
value(v) ::= builtin(b). { v = value_builtin(b); }

%type number { value_t* }
number(n) ::= TK_NUMBER(token). {
  n = value_number(token->number);
}

%type lambda { value_t* }
lambda(l) ::= TK_LPAREN TK_LAMBDA TK_IDENTIFIER(token_arg) TK_DOT body(lam_body) TK_RPAREN. {
  l = value_lambda(*token_arg->identifier, lam_body);
}

%type builtin { builtin_t* }
builtin(b) ::= TK_MULT. { b = builtin_mult(); }
builtin(b) ::= TK_SIN. { b = builtin_sin(); }

