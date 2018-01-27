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
%token <token> TK_WORD_OF TK_RARROW TK_NUMBER TK_LAMBDA TK_DOT TK_WORD_END
%token <token> TK_ANY TK_BUILTIN_SIN TK_BUILTIN_EXP TK_BUILTIN_INV
%token <token> TK_BUILTIN_PLUS TK_BUILTIN_MINUS TK_BUILTIN_MULT TK_BUILTIN_DIVIDE
%token <token> TK_BUILTIN_ABS TK_BUILTIN_FLOOR TK_BUILTIN_ROUND TK_BUILTIN_CEIL
%token <token> TK_WORD_IF TK_WORD_THEN TK_WORD_ELSE
%token <token> TK_OP_PLUS TK_OP_MINUS TK_OP_MULT TK_OP_DIVIDE TK_OP_CEQ
%token <token> TK_OP_CNEQ TK_OP_CLT TK_OP_CLTEQ TK_OP_CGT TK_OP_CGTEQ
%token <token> TK_OP_MOD

%left TK_OP_MINUS TK_OP_PLUS
%left TK_OP_MULT TK_OP_DIVIDE
%left TK_OP_CEQ TK_OP_CNEQ TK_OP_CLT TK_OP_CLTEQ TK_OP_CGT TK_OP_CGTEQ
%left TK_OP_MOD

%type <term> program definition body simple identifier case_of case_value;
%type <term> if_else binary_op;
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
       | if_else { $$ = $1; }
       | value { $$ = term_value($1); }
       | binary_op
       | TK_LPAREN body TK_RPAREN { $$ = $2; };

binary_op : simple TK_OP_PLUS simple { // not the prettiest code
               $$ = term_application(term_application(term_value(value_builtin(
               builtin_plus())), $1), $3);
           }
           | simple TK_OP_MINUS simple {
               $$ = term_application(term_application(term_value(value_builtin(
               builtin_minus())), $1), $3);
           }
           | simple TK_OP_MULT simple {
               $$ = term_application(term_application(term_value(value_builtin(
               builtin_mult())), $1), $3);
           }
           | simple TK_OP_DIVIDE simple {
               $$ = term_application(term_application(term_value(value_builtin(
               builtin_divide())), $1), $3);
           }
           | simple TK_OP_CEQ simple {
               $$ = term_application(term_application(term_value(value_builtin(
               builtin_ceq())), $1), $3);
           }
           | simple TK_OP_CNEQ simple {
               $$ = term_application(term_application(term_value(value_builtin(
               builtin_cneq())), $1), $3);
           }
           | simple TK_OP_CLT simple {
               $$ = term_application(term_application(term_value(value_builtin(
               builtin_clt())), $1), $3);
           }
           | simple TK_OP_CLTEQ simple {
               $$ = term_application(term_application(term_value(value_builtin(
               builtin_clteq())), $1), $3);
           }
           | simple TK_OP_CGT simple {
               $$ = term_application(term_application(term_value(value_builtin(
               builtin_cgt())), $1), $3);
           }
           | simple TK_OP_CGTEQ simple {
               $$ = term_application(term_application(term_value(value_builtin(
               builtin_cgteq())), $1), $3);
           }
           | simple TK_OP_MOD simple {
               $$ = term_application(term_application(term_value(value_builtin(
               builtin_mod())), $1), $3);
           };

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

if_else : TK_WORD_IF body TK_WORD_THEN body TK_WORD_ELSE body TK_WORD_END {
          $$ = term_if_else($2, $4, $6);
        };

value : number { $$ = $1; }
      | lambda { $$ = $1; }
      | builtin { $$ = value_builtin($1); };

number : TK_NUMBER { $$ = value_number($1->number); };

lambda : TK_LPAREN TK_LAMBDA TK_IDENTIFIER TK_DOT body TK_RPAREN {
         $$ = value_lambda(*$3->identifier, $5);
       };

builtin : TK_BUILTIN_PLUS { $$ = builtin_plus(); }
        | TK_BUILTIN_MINUS { $$ = builtin_minus(); }
        | TK_BUILTIN_MULT { $$ = builtin_mult(); }
        | TK_BUILTIN_DIVIDE { $$ = builtin_divide(); }
        | TK_BUILTIN_SIN { $$ = builtin_sin(); }
        | TK_BUILTIN_EXP { $$ = builtin_exp(); }
        | TK_BUILTIN_INV { $$ = builtin_inv(); }
        | TK_BUILTIN_ABS { $$ = builtin_abs(); }
        | TK_BUILTIN_FLOOR { $$ = builtin_floor(); }
        | TK_BUILTIN_ROUND { $$ = builtin_round(); }
        | TK_BUILTIN_CEIL { $$ = builtin_ceil(); };

