%{
#include "lex.hh"
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
%token <token> TK_WORD_END TK_ANY TK_EXP

%type <term> program definition body simple identifier case_of case_value;
%type <term_list> definition_list identifier_list simple_list;
%type <case_statement_list> case_statement_list;
%type <case_statement> case_statement;
%type <value> value number lambda;
%type <builtin> builtin;

%%

program : definition_list { *root = term_program($1); };

definition_list : definition_list definition { $$->push_back($2); }
                | definition {
                  $$ = new std::vector<term_t*>;
                  $$->push_back($1);
                };

definition : TK_IDENTIFIER TK_EQUALS body TK_EOS {
             $$ = term_definition(*$1->identifier, $3);
           }
           | TK_IDENTIFIER identifier_list TK_EQUALS body TK_EOS {
             term_t *p = $4;
             for (int i = $2->size() - 1; i >= 0; --i)
               p = term_value(value_lambda(*$2->at(i)->identifier.name, p));
             $$ = term_definition(*$1->identifier, p);
             // delete $2? probably yes
           };

identifier_list : identifier_list identifier { $$->push_back($2); }
                | identifier {
                  $$ = new std::vector<term_t*>;
                  $$->push_back($1);
                };

identifier : TK_IDENTIFIER { $$ = term_identifier(*$1->identifier); };

body : simple_list {
       term_t *p = $1->at(0);
       for (size_t i = 1; i < $1->size(); ++i)
         p = term_application(p, $1->at(i));
       $$ = p;
       // delete $1
     };

simple_list : simple_list simple { $$->push_back($2); }
            | simple {
              $$ = new std::vector<term_t*>;
              $$->push_back($1);
            };

simple : identifier { $$ = $1; }
       | case_of { $$ = $1; }
       | value { $$ = term_value($1); }
       | TK_LPAREN body TK_RPAREN { $$ = $2; };

case_of : TK_WORD_CASE body TK_WORD_OF case_statement_list TK_WORD_END {
          $$ = term_case_of($2, $4);
        };

case_statement_list : case_statement_list TK_EOS case_statement {
                      $$->push_back(*$3);
                      delete $3;
                    }
                    | case_statement {
                      $$ = new std::vector<term_t::case_statement>;
                      $$->push_back(*$1);
                      delete $1;
                    };

case_statement : case_value TK_RARROW body {
                 $$ = new term_t::case_statement { $1, $3 };
               };

case_value : body { $$ = $1; }
           | TK_ANY { $$ = nullptr; };

value : number { $$ = $1; }
      | lambda { $$ = $1; }
      | builtin { $$ = value_builtin($1); };

number : TK_NUMBER { $$ = value_number($1->number); };

lambda : TK_LPAREN TK_LAMBDA TK_IDENTIFIER TK_DOT body TK_RPAREN {
         $$ = value_lambda(*$3->identifier, $5);
       };

builtin : TK_MULT { $$ = builtin_mult(); }
        | TK_SIN { $$ = builtin_sin(); }
        | TK_EXP { $$ = builtin_exp(); };

