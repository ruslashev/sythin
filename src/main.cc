#include "eval.hh"
#include "parse.hh"
#include "lang.hh"
#include "utils.hh"
#include "wav_writer.hh"
#include <cmath>

int main() {
  // these functions are unused (and builtins are used instead) because
  // they blow up stack becase of level of recursion of evaluate_term()s
  // etc. when used on large numbers

  // add = (\x . (\y .
  // case y of
  //  0 -> x
  //  _ -> (succ (add x (pred y)))
  // ))
  // mult = (\x . (\y .
  // case y of
  //  0 -> x
  //  _ -> (add ((mult x) (pred y)) x)
  // ))

  std::string source =
    "two = 2\n"
    "sin_alias = sin\n"
    "double = (mult two)\n"
    "mult3 = (\\ x . ( \\y . ( \\z . (mult ((mult x) y) z))))\n"
    "mult2pi2 = (mult3 (double pi))\n"
    "main = (\\ f . (λ t . (sin_alias ((mult2pi2 f) t))\n";

  printf("source \"%s\"\n", source.c_str());

  lexer_t lexer;
  lexer.from_string(source);

  token_t *t;
  do {
    t = lexer.next_token();
    t->pretty_print();
  } while (t->kind != token_k::eof);

#if 0
  term_t *program = term_program(new std::vector<term_t*>{
    term_definition("two",
      term_value(value_number(2))
    ),
    term_definition("sin_alias",
      term_value(value_builtin(builtin_sin()))
    ),
    term_definition("double",
      term_application(
        term_value(value_builtin(builtin_mult())),
        term_identifier("two")
      )
    ),
    term_definition("mult3",
      term_value(value_lambda("x",
        term_value(value_lambda("y",
          term_value(value_lambda("z",
            term_application(
              term_application(
                term_value(value_builtin(builtin_mult())),
                term_application(
                  term_application(
                    term_value(value_builtin(builtin_mult())),
                    term_identifier("x")
                  ),
                  term_identifier("y")
                )
              ),
              term_identifier("z")
            )
          ))
        ))
      ))
    ),
    term_definition("mult2pi2",
      term_application(
        term_identifier("mult3"),
        term_application(
          term_identifier("double"),
          term_identifier("pi")
        )
      )
    ),
    term_definition("main",
      term_value(value_lambda("f",
        term_value(value_lambda("t",
          term_application(
            term_identifier("sin_alias"),
            term_application(
              term_application(
                term_identifier("mult2pi2"),
                term_identifier("f")
              ),
              term_identifier("t")
            )
          )
        ))
      ))
    )
  });
  scope_t global_scope {
    { "pi", value_number(M_PI) }
  };
  program->scope = &global_scope;

  program->pretty_print();

  std::vector<message_t> messages;
  validate_top_level_functions(program, &messages);
  for (const message_t &message : messages)
    printf("%s: %s\n", message_kind_to_string(message.kind).c_str()
        , message.content.c_str());
  assertf(messages_contain_no_errors(messages));

  samples_t samples = { std::vector<uint16_t>(), std::vector<uint16_t>() };

  double amplitude = 32760;
  double sample_rate = 44100;
  double frequency = 261.626; // C4
  double seconds = 2.5;

  uint64_t num_samples = sample_rate * seconds;
  double inv_sample_rate = 1. / (double)sample_rate;
  for (uint64_t i = 0; i < num_samples; i++) {
    double f = frequency;
    double t = (double)i * inv_sample_rate;
    double value = evaluate_program(program, f, t);
    uint16_t w_value = amplitude * value;
    samples[0].push_back(w_value);
    samples[1].push_back(w_value);
  }

  write_wav("out.wav", sample_rate, samples);

  for (const auto &scope_pair : global_scope)
    delete scope_pair.second;
  program->scope = nullptr;
  delete program;
#endif
}

