%include {
#include "parse.hh"
#include <cassert>
}

%token_type { token_t* }

program ::= definition_list.

definition_list ::= TK_IDENTIFIER. { puts("ident"); }

