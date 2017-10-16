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

body(b) ::= other(lambda) other(parameter). {
  b = term_application(lambda, parameter);
}
body(b) ::= other(o). { b = o; }

other(o) ::= identifier(i). { o = i; }
other(o) ::= case_of(c). { o = c; }
other(o) ::= value(v). { o = term_value(v); }
other(o) ::= TK_LPAREN body(par_body) TK_RPAREN. { o = par_body; }

identifier(i) ::= TK_IDENTIFIER(token). {
  i = term_identifier(*token->identifier);
}

case_of(c) ::= TK_WORD_CASE body(case_value) TK_WORD_OF
    case_statement_list(statements) TK_WORD_END. {
  c = term_case_of(case_value, statements);
}

%type case_statement_list { std::vector<term_t::case_statement>* }
case_statement_list(c) ::= case_statement_list case_statement(case_stmt). {
  c->push_back(*case_stmt);
  delete case_stmt;
}
case_statement_list(c) ::= case_statement(case_stmt). {
  c = new std::vector<term_t::case_statement>;
  c->push_back(*case_stmt);
  delete case_stmt;
}

%type case_statement { term_t::case_statement* }
case_statement(c) ::= body(case_stament_value) TK_RARROW
    body(case_stament_result) TK_EOS. {
  c = new term_t::case_statement;
  c->value = case_stament_value;
  c->result = case_stament_result;
}

%type value { value_t* }
value(v) ::= number(n). { v = n; }
value(v) ::= lambda(l). { v = l; }
value(v) ::= builtin(b). { v = value_builtin(b); }

%type number { value_t* }
number(n) ::= TK_NUMBER(token). {
  n = value_number(token->number);
}

%type lambda { value_t* }
lambda(l) ::= TK_LPAREN TK_LAMBDA TK_IDENTIFIER(token_arg) TK_DOT body(lam_body)
    TK_RPAREN. {
  l = value_lambda(*token_arg->identifier, lam_body);
}

%type builtin { builtin_t* }
builtin(b) ::= TK_MULT. { b = builtin_mult(); }
builtin(b) ::= TK_SIN. { b = builtin_sin(); }

