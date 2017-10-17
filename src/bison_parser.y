%{
#include "parse.hh"
%}

%define api.pure full
%define api.value.type { yyunion_t }
%define parse.lac full
%define parse.error verbose
%output "bison_parser.cc"
%defines "bison_parser_tokens.hh"
%param { lexer_t *lexer }
%parse-param { term_t **root }

%token <token> TK_IDENTIFIER TK_EQUALS TK_EOS TK_LPAREN TK_RPAREN TK_WORD_CASE
%token <token> TK_WORD_OF TK_RARROW TK_NUMBER TK_LAMBDA TK_DOT TK_MULT TK_SIN
%token <token> TK_WORD_END

%type <term> program definition body application identifier case_of;
%type <term_list> definition_list;
%type <case_statement_list> case_statement_list;
%type <case_statement> case_statement;
%type <value> value number lambda;
%type <builtin> builtin;

%%

program : definition_list { *root = term_program($1); };

definition_list : definition_list definition {
                  if ($2)
                    $$->push_back($2);
                }
                | definition {
                  $$ = new std::vector<term_t*>;
                  if ($1)
                    $$->push_back($1);
                };

definition : TK_IDENTIFIER TK_EQUALS body TK_EOS {
             $$ = term_definition(*$1->identifier, $3);
           }
           | TK_EOS { $$ = nullptr; };

body: application { $$ = $1; }
    | identifier { $$ = $1; }
    | case_of { $$ = $1; }
    | value { $$ = term_value($1); }
    | TK_LPAREN body TK_RPAREN { $$ = $2; };

application: TK_LPAREN body body TK_RPAREN { $$ = term_application($2, $3); };

/* body : other other { $$ = term_application($1, $2); } */
/*      | other { $$ = $1; }; */

/* other : identifier { $$ = $1; } */
/*       | value { $$ = term_value($1); } */
/*       | case_of { $$ = $1; } */
/*       | TK_LPAREN body TK_RPAREN { $$ = $2; }; */

identifier : TK_IDENTIFIER { $$ = term_identifier(*$1->identifier); };

case_of : TK_WORD_CASE body TK_WORD_OF case_statement_list TK_WORD_END {
          $$ = term_case_of($2, $4);
        };

case_statement_list : case_statement_list case_statement {
                      $$->push_back(*$2);
                      delete $2;
                    }
                    | case_statement {
                      $$ = new std::vector<term_t::case_statement>;
                      $$->push_back(*$1);
                      delete $1;
                    };

case_statement : body TK_RARROW body TK_EOS {
                 $$ = new term_t::case_statement { $1, $3 };
               };

value : number { $$ = $1; }
      | lambda { $$ = $1; }
      | builtin { $$ = value_builtin($1); };

number : TK_NUMBER { $$ = value_number($1->number); };

lambda : TK_LPAREN TK_LAMBDA TK_IDENTIFIER TK_DOT body TK_RPAREN {
         $$ = value_lambda(*$3->identifier, $5);
       };

builtin : TK_MULT { $$ = builtin_mult(); }
        | TK_SIN { $$ = builtin_sin(); };

