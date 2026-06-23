#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SANTRON_VM_NO_MAIN
#include "santron-vm.c"

#define MAX_CELLS 72
#define MAX_OPS 256
#define MAX_PROGRAM_OPS 72

typedef struct {
  double encoded;
  int guess;
  int pos;
  int black0;
  int m9_0;
} TestCase;

typedef struct {
  int code[4];
  int guess[4];
} SequenceTest;

typedef struct {
  uint16_t codes[3];
  int len;
} Operation;

typedef struct {
  Operation ops[MAX_PROGRAM_OPS];
  int op_count;
} Program;

typedef struct {
  int exact;
  int total;
  double error;
  double penalty;
  double cost;
  int cells;
} Score;

static const TestCase TRAIN_TESTS[] = {
  {1024030, 1, 1, 0, 4}, {1024030, 2, 2, 0, 4},
  {1024030, 3, 3, 1, 3}, {1024030, 4, 4, 1, 3},
  {1024030, 5, 1, 2, 2}, {1024030, 6, 2, 2, 2},
  {3040210, 1, 1, 0, 4}, {3040210, 2, 2, 1, 3},
  {3040210, 3, 3, 1, 3}, {3040210, 4, 4, 2, 2},
  {201034, 1, 3, 0, 4}, {201034, 3, 1, 0, 4},
  {201034, 5, 2, 1, 3}, {201034, 6, 4, 1, 3},
  {1234560, 1, 4, 0, 4}, {1234560, 6, 1, 0, 4},
};

static const int TRAIN_COUNT = (int)(sizeof(TRAIN_TESTS) / sizeof(TRAIN_TESTS[0]));

static const SequenceTest TRAIN_SEQUENCES[] = {
  {{1, 2, 3, 4}, {1, 2, 3, 4}},
  {{1, 2, 3, 4}, {4, 3, 2, 1}},
  {{6, 4, 1, 3}, {1, 2, 3, 4}},
  {{6, 4, 1, 3}, {6, 4, 1, 3}},
  {{1, 2, 6, 4}, {3, 5, 1, 4}},
  {{3, 5, 1, 4}, {1, 2, 6, 4}},
};

static const int TRAIN_SEQUENCE_COUNT = (int)(sizeof(TRAIN_SEQUENCES) / sizeof(TRAIN_SEQUENCES[0]));

static uint64_t rng_state = 0x123456789abcdef0ULL;

static uint64_t rng_next(void) {
  uint64_t x = rng_state;
  x ^= x << 13;
  x ^= x >> 7;
  x ^= x << 17;
  rng_state = x;
  return x;
}

static int rng_int(int limit) {
  return limit <= 0 ? 0 : (int)(rng_next() % (uint64_t)limit);
}

static double rng_double(void) {
  return (double)(rng_next() >> 11) * (1.0 / 9007199254740992.0);
}

static bool nearly_equal(double a, double b) {
  if (isnan(a) && isnan(b)) return true;
  return fabs(a - b) <= 1e-8;
}

static int digit_at(double encoded, int place) {
  long long scale = 1;
  for (int i = 0; i < place; i++) scale *= 10;
  long long value = llround(encoded);
  return (int)((value / scale) % 10);
}

static bool has_duplicate_digits(const int digits[4]) {
  for (int i = 0; i < 4; i++) {
    for (int j = i + 1; j < 4; j++) {
      if (digits[i] == digits[j]) return true;
    }
  }
  return false;
}

static int encode_code(const int code[4]) {
  static const int pow10[] = {
    1, 10, 100, 1000, 10000, 100000, 1000000
  };
  int encoded = 0;
  for (int pos = 0; pos < 4; pos++) {
    encoded += (pos + 1) * pow10[code[pos]];
  }
  return encoded;
}

static int program_cell_count(const Program *p) {
  int cells = 0;
  for (int i = 0; i < p->op_count; i++) cells += p->ops[i].len;
  return cells;
}

static int flatten_program(const Program *p, uint16_t *codes, int max_codes) {
  int count = 0;
  for (int i = 0; i < p->op_count; i++) {
    for (int j = 0; j < p->ops[i].len; j++) {
      if (count >= max_codes) return count;
      codes[count++] = p->ops[i].codes[j];
    }
  }
  return count;
}

static void load_candidate(SantronState *s, const uint16_t *codes, int len) {
  for (int i = 0; i < PROGRAM_SIZE; i++) s->program[i] = KEY_BLANK;
  for (int i = 0; i < len && i < PROGRAM_SIZE; i++) s->program[i] = (int)codes[i];
  if (len + 3 < PROGRAM_SIZE) {
    s->program[len] = KEY_RCL;
    s->program[len + 1] = KEY_DIGIT0 + 2;
    s->program[len + 2] = KEY_EQ;
    s->program[len + 3] = KEY_RS;
  } else if (len < PROGRAM_SIZE) {
    s->program[len] = KEY_RS;
  }
  s->pc = 0;
}

static void expected_state(const TestCase *test, int *black, int *m9, int *pos) {
  int code_pos = digit_at(test->encoded, test->guess);
  int is_hit = code_pos != 0;
  int is_black = code_pos == test->pos;
  *black = test->black0 + is_black;
  *m9 = test->m9_0 - (is_hit ? 0 : 1) - (is_black ? 1 : 0);
  *pos = test->pos + 1;
}

static bool loop_sign_ok(const SantronState *s, int pos_after_step) {
  return pos_after_step == 5 ? s->x < 0.0 : s->x >= 0.0;
}

static void expected_score(const int code[4], const int guess[4], int *black, int *white) {
  int hits = 0;
  *black = 0;

  for (int i = 0; i < 4; i++) {
    if (code[i] == guess[i]) (*black)++;
    for (int j = 0; j < 4; j++) {
      if (guess[i] == code[j]) hits++;
    }
  }

  *white = hits - *black;
}

static void setup_state(SantronState *s, const TestCase *test, const double scratch[4], const uint16_t *codes, int len) {
  init_state(s);
  load_candidate(s, codes, len);
  s->x = 0.0;
  s->entering = false;
  s->memory[0] = 1.0;
  s->memory[1] = (double)test->black0;
  s->memory[2] = (double)test->pos;
  s->memory[3] = scratch[0];
  s->memory[4] = scratch[1];
  s->memory[5] = scratch[2];
  s->memory[6] = scratch[3];
  s->memory[7] = scratch[0] - scratch[3];
  s->memory[8] = test->encoded;
  s->memory[9] = (double)test->m9_0;
  execute_key(s, KEY_DIGIT0 + test->guess, false);
}

static void setup_sequence_state(SantronState *s, int encoded, const double scratch[4], const uint16_t *codes, int len) {
  init_state(s);
  load_candidate(s, codes, len);
  s->x = 0.0;
  s->entering = false;
  s->memory[0] = 1.0;
  s->memory[1] = 0.0;
  s->memory[2] = 1.0;
  s->memory[3] = scratch[0];
  s->memory[4] = scratch[1];
  s->memory[5] = scratch[2];
  s->memory[6] = scratch[3];
  s->memory[7] = scratch[0] - scratch[3];
  s->memory[8] = (double)encoded;
  s->memory[9] = 4.0;
}

static void enter_guess_digit(SantronState *s, int digit) {
  s->pc = 0;
  s->paused = false;
  execute_key(s, KEY_DIGIT0 + digit, false);
}

static Score score_program(const Program *p, const TestCase *tests, int test_count) {
  uint16_t codes[MAX_CELLS];
  int len = flatten_program(p, codes, MAX_CELLS);
  Score score = {0};
  score.cells = len;
  const double scratch_values[][4] = {
    {30.0, 50.0, 0.0, 60.0},
    {0.0, 0.0, 7.0, 0.0},
    {1.0, 10.0, -2.0, -3.0},
  };

  for (int i = 0; i < test_count; i++) {
    int expected_black = 0;
    int expected_m9 = 0;
    int expected_pos = 0;
    expected_state(&tests[i], &expected_black, &expected_m9, &expected_pos);

    for (size_t scratch = 0; scratch < sizeof(scratch_values) / sizeof(scratch_values[0]); scratch++) {
      SantronState s;
      setup_state(&s, &tests[i], scratch_values[scratch], codes, len);
      run_program(&s, 1000);
      score.total++;

      bool ok = true;
      ok = ok && nearly_equal(s.memory[0], 1.0);
      ok = ok && nearly_equal(s.memory[1], (double)expected_black);
      ok = ok && nearly_equal(s.memory[2], (double)expected_pos);
      ok = ok && nearly_equal(s.memory[8], tests[i].encoded);
      ok = ok && nearly_equal(s.memory[9], (double)expected_m9);
      ok = ok && s.pending == OP_NONE;
      ok = ok && s.paren_depth == 0;
      ok = ok && s.pending_memory < 0;
      ok = ok && !s.exponent_entry;
      ok = ok && s.paused;
      ok = ok && loop_sign_ok(&s, expected_pos);
      if (ok) score.exact++;

      score.error += fabs(s.memory[1] - (double)expected_black);
      score.error += fabs(s.memory[2] - (double)expected_pos);
      score.error += fabs(s.memory[9] - (double)expected_m9);
      if (!loop_sign_ok(&s, expected_pos)) score.penalty += 1000.0;
      if (!nearly_equal(s.memory[0], 1.0)) score.penalty += 1000.0;
      if (!nearly_equal(s.memory[8], tests[i].encoded)) score.penalty += 1000.0;
      if (s.pending != OP_NONE) score.penalty += 1000.0;
      if (s.paren_depth != 0) score.penalty += 1000.0;
      if (s.pending_memory >= 0) score.penalty += 100.0;
      if (s.exponent_entry) score.penalty += 100.0;
      if (!s.paused) score.penalty += 1000.0;
    }
  }

  for (int i = 0; i < TRAIN_SEQUENCE_COUNT; i++) {
    int encoded = encode_code(TRAIN_SEQUENCES[i].code);
    int expected_black = 0;
    int expected_white = 0;
    expected_score(TRAIN_SEQUENCES[i].code, TRAIN_SEQUENCES[i].guess, &expected_black, &expected_white);

    for (size_t scratch = 0; scratch < sizeof(scratch_values) / sizeof(scratch_values[0]); scratch++) {
      SantronState s;
      setup_sequence_state(&s, encoded, scratch_values[scratch], codes, len);
      bool loop_ok = true;
      for (int pos = 0; pos < 4; pos++) {
        enter_guess_digit(&s, TRAIN_SEQUENCES[i].guess[pos]);
        run_program(&s, 1000);
        loop_ok = loop_ok && loop_sign_ok(&s, pos + 2);
      }
      score.total++;

      bool ok = true;
      ok = ok && nearly_equal(s.memory[0], 1.0);
      ok = ok && nearly_equal(s.memory[1], (double)expected_black);
      ok = ok && nearly_equal(s.memory[2], 5.0);
      ok = ok && nearly_equal(s.memory[8], (double)encoded);
      ok = ok && nearly_equal(s.memory[9], (double)expected_white);
      ok = ok && s.pending == OP_NONE;
      ok = ok && s.paren_depth == 0;
      ok = ok && s.pending_memory < 0;
      ok = ok && !s.exponent_entry;
      ok = ok && s.paused;
      ok = ok && loop_ok;
      if (ok) score.exact++;

      score.error += fabs(s.memory[1] - (double)expected_black);
      score.error += fabs(s.memory[2] - 5.0);
      score.error += fabs(s.memory[9] - (double)expected_white);
      if (!loop_ok) score.penalty += 1000.0;
      if (!nearly_equal(s.memory[0], 1.0)) score.penalty += 1000.0;
      if (!nearly_equal(s.memory[8], (double)encoded)) score.penalty += 1000.0;
      if (s.pending != OP_NONE) score.penalty += 1000.0;
      if (s.paren_depth != 0) score.penalty += 1000.0;
      if (s.pending_memory >= 0) score.penalty += 100.0;
      if (s.exponent_entry) score.penalty += 100.0;
      if (!s.paused) score.penalty += 1000.0;
    }
  }

  score.cost =
    (double)(score.total - score.exact) * 100000.0 +
    score.error * 100.0 +
    score.penalty +
    (double)score.cells;
  return score;
}

static bool validate_all(const Program *p) {
  const double scratch_values[][4] = {
    {33.0, 55.0, 0.0, 66.0},
    {0.0, 0.0, 7.0, 0.0},
    {1.0, 10.0, -2.0, -3.0},
  };
  uint16_t codes[MAX_CELLS];
  int len = flatten_program(p, codes, MAX_CELLS);

  for (int c0 = 1; c0 <= 6; c0++) {
    for (int c1 = 1; c1 <= 6; c1++) {
      for (int c2 = 1; c2 <= 6; c2++) {
        for (int c3 = 1; c3 <= 6; c3++) {
          int code_digits[4] = {c0, c1, c2, c3};
          if (has_duplicate_digits(code_digits)) continue;
          double encoded = (double)encode_code(code_digits);
          for (int guess = 1; guess <= 6; guess++) {
            for (int pos = 1; pos <= 4; pos++) {
              for (int black0 = 0; black0 <= 3; black0++) {
                for (int m9_0 = 0; m9_0 <= 4; m9_0++) {
                  TestCase test = {encoded, guess, pos, black0, m9_0};
                  int expected_black, expected_m9, expected_pos;
                  expected_state(&test, &expected_black, &expected_m9, &expected_pos);
                  for (size_t scratch = 0; scratch < sizeof(scratch_values) / sizeof(scratch_values[0]); scratch++) {
                    SantronState s;
                    setup_state(&s, &test, scratch_values[scratch], codes, len);
                    run_program(&s, 1000);
                    if (!nearly_equal(s.memory[0], 1.0)) return false;
                    if (!nearly_equal(s.memory[1], (double)expected_black)) return false;
                    if (!nearly_equal(s.memory[2], (double)expected_pos)) return false;
                    if (!nearly_equal(s.memory[8], test.encoded)) return false;
                    if (!nearly_equal(s.memory[9], (double)expected_m9)) return false;
                    if (s.pending != OP_NONE || s.paren_depth != 0 || s.pending_memory >= 0 || s.exponent_entry) return false;
                    if (!s.paused) return false;
                    if (!loop_sign_ok(&s, expected_pos)) return false;
                  }
                }
              }
            }
          }
        }
      }
    }
  }

  for (int c0 = 1; c0 <= 6; c0++) {
    for (int c1 = 1; c1 <= 6; c1++) {
      for (int c2 = 1; c2 <= 6; c2++) {
        for (int c3 = 1; c3 <= 6; c3++) {
          int code_digits[4] = {c0, c1, c2, c3};
          if (has_duplicate_digits(code_digits)) continue;
          int encoded = encode_code(code_digits);
          for (int g0 = 1; g0 <= 6; g0++) {
            for (int g1 = 1; g1 <= 6; g1++) {
              for (int g2 = 1; g2 <= 6; g2++) {
                for (int g3 = 1; g3 <= 6; g3++) {
                  int guess_digits[4] = {g0, g1, g2, g3};
                  if (has_duplicate_digits(guess_digits)) continue;
                  int expected_black, expected_white;
                  expected_score(code_digits, guess_digits, &expected_black, &expected_white);
                  for (size_t scratch = 0; scratch < sizeof(scratch_values) / sizeof(scratch_values[0]); scratch++) {
                    SantronState s;
                    setup_sequence_state(&s, encoded, scratch_values[scratch], codes, len);
                    bool loop_ok = true;
                    for (int pos = 0; pos < 4; pos++) {
                      enter_guess_digit(&s, guess_digits[pos]);
                      run_program(&s, 1000);
                      loop_ok = loop_ok && loop_sign_ok(&s, pos + 2);
                    }
                    if (!nearly_equal(s.memory[0], 1.0)) return false;
                    if (!nearly_equal(s.memory[1], (double)expected_black)) return false;
                    if (!nearly_equal(s.memory[2], 5.0)) return false;
                    if (!nearly_equal(s.memory[8], (double)encoded)) return false;
                    if (!nearly_equal(s.memory[9], (double)expected_white)) return false;
                    if (s.pending != OP_NONE || s.paren_depth != 0 || s.pending_memory >= 0 || s.exponent_entry) return false;
                    if (!s.paused) return false;
                    if (!loop_ok) return false;
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  return true;
}

static int full_operation_set(Operation *ops, int capacity) {
  const int single_cell[] = {
    KEY_ADD, KEY_SUB, KEY_MUL, KEY_DIV, KEY_INV, KEY_SQR, KEY_SQRT,
    KEY_CLEAR, KEY_SIGN, KEY_EXP, KEY_SWAP, KEY_PI, KEY_DOT,
    KEY_EQ, KEY_LPAREN, KEY_RPAREN, KEY_SKP, KEY_TENX
  };
  const int memory_commands[] = {
    KEY_MPLUS, KEY_MMINUS, KEY_MULMEM, KEY_DIVMEM, KEY_STO, KEY_RCL
  };

  int count = 0;
  for (size_t i = 0; i < sizeof(single_cell) / sizeof(single_cell[0]); i++) {
    if (count >= capacity) return count;
    ops[count].codes[0] = (uint16_t)single_cell[i];
    ops[count].len = 1;
    count++;
  }
  for (int digit = 0; digit <= 9; digit++) {
    if (count >= capacity) return count;
    ops[count].codes[0] = (uint16_t)(KEY_DIGIT0 + digit);
    ops[count].len = 1;
    count++;
  }
  for (size_t i = 0; i < sizeof(memory_commands) / sizeof(memory_commands[0]); i++) {
    for (int digit = 0; digit <= 9; digit++) {
      if (count >= capacity) return count;
      ops[count].codes[0] = (uint16_t)memory_commands[i];
      ops[count].codes[1] = (uint16_t)(KEY_DIGIT0 + digit);
      ops[count].len = 2;
      count++;
    }
  }
  return count;
}

static const char *code_name(int code) {
  switch (code) {
    case KEY_ADD: return "+";
    case KEY_SUB: return "-";
    case KEY_MUL: return "*";
    case KEY_DIV: return "/";
    case KEY_INV: return "1/X";
    case KEY_SQR: return "X^2";
    case KEY_SQRT: return "SQRT";
    case KEY_TENX: return "10^X";
    case KEY_MPLUS: return "M+";
    case KEY_MMINUS: return "M-";
    case KEY_MULMEM: return "M*";
    case KEY_DIVMEM: return "M/";
    case KEY_STO: return "STO";
    case KEY_RCL: return "RCL";
    case KEY_CLEAR: return "C/CE";
    case KEY_SIGN: return "+/-";
    case KEY_EXP: return "EXP";
    case KEY_SWAP: return "X<>Y";
    case KEY_PI: return "PI";
    case KEY_DOT: return ".";
    case KEY_EQ: return "=";
    case KEY_LPAREN: return "(";
    case KEY_RPAREN: return ")";
    case KEY_RS: return "R/S";
    case KEY_SKP: return "SKP";
    default: break;
  }
  static char digit_names[10][2] = {
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9"
  };
  int digit = digit_from_code(code);
  if (digit >= 0) return digit_names[digit];
  return "?";
}

static void print_program(const Program *p, const Score *score) {
  uint16_t codes[MAX_CELLS];
  int len = flatten_program(p, codes, MAX_CELLS);
  printf("cells=%d ops=%d exact=%d/%d error=%.12g penalty=%.12g cost=%.12g\n",
         len, p->op_count, score->exact, score->total, score->error,
         score->penalty, score->cost);
  for (int i = 0; i < len; i++) printf("%s%s", i ? " " : "", code_name(codes[i]));
  printf("\n");
  for (int i = 0; i < len; i++) printf("%02d %03d %s\n", i, codes[i], code_name(codes[i]));
}

static void seed_step(Program *p) {
  memset(p, 0, sizeof(*p));
  const Operation seed[] = {
    {{KEY_STO, KEY_DIGIT0 + 4, 0}, 2},
    {{KEY_DIGIT0 + 1, 0, 0}, 1},
    {{KEY_DIGIT0 + 0, 0, 0}, 1},
    {{KEY_STO, KEY_DIGIT0 + 6, 0}, 2},
    {{KEY_RCL, KEY_DIGIT0 + 8, 0}, 2},
    {{KEY_DIV, 0, 0}, 1},
    {{KEY_RCL, KEY_DIGIT0 + 4, 0}, 2},
    {{KEY_TENX, 0, 0}, 1},
    {{KEY_ADD, 0, 0}, 1},
    {{KEY_EXP, 0, 0}, 1},
    {{KEY_DIGIT0 + 0, 0, 0}, 1},
    {{KEY_DIGIT0 + 9, 0, 0}, 1},
    {{KEY_ADD, 0, 0}, 1},
    {{KEY_STO, KEY_DIGIT0 + 7, 0}, 2},
    {{KEY_MULMEM, KEY_DIGIT0 + 6, 0}, 2},
    {{KEY_RCL, KEY_DIGIT0 + 6, 0}, 2},
    {{KEY_SUB, 0, 0}, 1},
    {{KEY_RCL, KEY_DIGIT0 + 6, 0}, 2},
    {{KEY_EQ, 0, 0}, 1},
    {{KEY_MMINUS, KEY_DIGIT0 + 7, 0}, 2},
    {{KEY_RCL, KEY_DIGIT0 + 7, 0}, 2},
    {{KEY_SUB, 0, 0}, 1},
    {{KEY_SIGN, 0, 0}, 1},
    {{KEY_SKP, 0, 0}, 1},
    {{KEY_RCL, KEY_DIGIT0 + 0, 0}, 2},
    {{KEY_MMINUS, KEY_DIGIT0 + 9, 0}, 2},
    {{KEY_RCL, KEY_DIGIT0 + 2, 0}, 2},
    {{KEY_MUL, 0, 0}, 1},
    {{KEY_SIGN, 0, 0}, 1},
    {{KEY_SUB, 0, 0}, 1},
    {{KEY_DIGIT0 + 4, 0, 0}, 1},
    {{KEY_SWAP, 0, 0}, 1},
    {{KEY_SKP, 0, 0}, 1},
    {{KEY_RCL, KEY_DIGIT0 + 0, 0}, 2},
    {{KEY_MMINUS, KEY_DIGIT0 + 9, 0}, 2},
    {{KEY_MPLUS, KEY_DIGIT0 + 1, 0}, 2},
    {{KEY_DIGIT0 + 1, 0, 0}, 1},
    {{KEY_MPLUS, KEY_DIGIT0 + 2, 0}, 2},
  };
  p->op_count = (int)(sizeof(seed) / sizeof(seed[0]));
  memcpy(p->ops, seed, sizeof(seed));
}

static void mutate_program(Program *dst, const Program *src, const Operation *ops, int op_count) {
  *dst = *src;
  int mutation = rng_int(7);

  if (mutation == 0 && dst->op_count > 0) {
    int idx = rng_int(dst->op_count);
    for (int i = idx; i + 1 < dst->op_count; i++) dst->ops[i] = dst->ops[i + 1];
    dst->op_count--;
  } else if (mutation == 1 && dst->op_count < MAX_PROGRAM_OPS) {
    int idx = rng_int(dst->op_count + 1);
    for (int i = dst->op_count; i > idx; i--) dst->ops[i] = dst->ops[i - 1];
    dst->ops[idx] = ops[rng_int(op_count)];
    dst->op_count++;
  } else if (mutation == 2 && dst->op_count > 0) {
    dst->ops[rng_int(dst->op_count)] = ops[rng_int(op_count)];
  } else if (mutation == 3 && dst->op_count > 1) {
    int a = rng_int(dst->op_count);
    int b = rng_int(dst->op_count);
    Operation tmp = dst->ops[a];
    dst->ops[a] = dst->ops[b];
    dst->ops[b] = tmp;
  } else if (mutation == 4 && dst->op_count > 1 && dst->op_count < MAX_PROGRAM_OPS) {
    int idx = rng_int(dst->op_count);
    for (int i = dst->op_count; i > idx; i--) dst->ops[i] = dst->ops[i - 1];
    dst->op_count++;
  } else if (mutation == 5 && dst->op_count > 1) {
    int idx = rng_int(dst->op_count - 1);
    dst->ops[idx] = dst->ops[idx + 1];
  } else if (dst->op_count > 0) {
    int idx = rng_int(dst->op_count);
    if (dst->ops[idx].len == 2) {
      dst->ops[idx].codes[1] = (uint16_t)(KEY_DIGIT0 + rng_int(10));
    } else {
      dst->ops[idx] = ops[rng_int(op_count)];
    }
  }

  while (program_cell_count(dst) > MAX_CELLS && dst->op_count > 0) {
    int idx = rng_int(dst->op_count);
    for (int i = idx; i + 1 < dst->op_count; i++) dst->ops[i] = dst->ops[i + 1];
    dst->op_count--;
  }
}

static int parse_int_arg(int argc, char **argv, const char *name, int default_value) {
  for (int i = 1; i + 1 < argc; i++) {
    if (strcmp(argv[i], name) == 0) return atoi(argv[i + 1]);
  }
  return default_value;
}

static bool has_flag(int argc, char **argv, const char *name) {
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], name) == 0) return true;
  }
  return false;
}

static void seed_user_candidate(Program *p) {
  memset(p, 0, sizeof(*p));
  const Operation seed[] = {
    {{KEY_TENX, 0, 0}, 1},
    {{KEY_DIV, 0, 0}, 1},
    {{KEY_RCL, KEY_DIGIT0 + 8, 0}, 2},
    {{KEY_SWAP, 0, 0}, 1},
    {{KEY_ADD, 0, 0}, 1},
    {{KEY_DIGIT0 + 9, 0, 0}, 1},
    {{KEY_EXP, 0, 0}, 1},
    {{KEY_DIGIT0 + 9, 0, 0}, 1},
    {{KEY_STO, KEY_DIGIT0 + 7, 0}, 2},
    {{KEY_MMINUS, KEY_DIGIT0 + 4, 0}, 2},
    {{KEY_ADD, 0, 0}, 1},
    {{KEY_MULMEM, KEY_DIGIT0 + 5, 0}, 2},
    {{KEY_STO, KEY_DIGIT0 + 4, 0}, 2},
    {{KEY_RCL, KEY_DIGIT0 + 7, 0}, 2},
    {{KEY_SUB, 0, 0}, 1},
    {{KEY_STO, KEY_DIGIT0 + 5, 0}, 2},
    {{KEY_INV, 0, 0}, 1},
    {{KEY_RCL, KEY_DIGIT0 + 4, 0}, 2},
    {{KEY_EQ, 0, 0}, 1},
    {{KEY_MMINUS, KEY_DIGIT0 + 7, 0}, 2},
    {{KEY_RCL, KEY_DIGIT0 + 7, 0}, 2},
    {{KEY_SUB, 0, 0}, 1},
    {{KEY_SIGN, 0, 0}, 1},
    {{KEY_SKP, 0, 0}, 1},
    {{KEY_RCL, KEY_DIGIT0 + 0, 0}, 2},
    {{KEY_MMINUS, KEY_DIGIT0 + 9, 0}, 2},
    {{KEY_RCL, KEY_DIGIT0 + 2, 0}, 2},
    {{KEY_DIV, 0, 0}, 1},
    {{KEY_ADD, 0, 0}, 1},
    {{KEY_SQR, 0, 0}, 1},
    {{KEY_SIGN, 0, 0}, 1},
    {{KEY_SKP, 0, 0}, 1},
    {{KEY_RCL, KEY_DIGIT0 + 0, 0}, 2},
    {{KEY_MPLUS, KEY_DIGIT0 + 1, 0}, 2},
    {{KEY_MMINUS, KEY_DIGIT0 + 9, 0}, 2},
    {{KEY_DIGIT0 + 1, 0, 0}, 1},
    {{KEY_MPLUS, KEY_DIGIT0 + 2, 0}, 2},
  };
  p->op_count = (int)(sizeof(seed) / sizeof(seed[0]));
  memcpy(p->ops, seed, sizeof(seed));
}

int main(int argc, char **argv) {
  const int iterations = parse_int_arg(argc, argv, "--iterations", 300000);
  const int seed = parse_int_arg(argc, argv, "--seed", (int)time(NULL));
  rng_state = (uint64_t)(unsigned int)seed;
  if (!rng_state) rng_state = 1;

  Operation ops[MAX_OPS];
  int op_count = full_operation_set(ops, MAX_OPS);

  Program current;
  if (has_flag(argc, argv, "--user-candidate")) seed_user_candidate(&current);
  else seed_step(&current);
  Score current_score = score_program(&current, TRAIN_TESTS, TRAIN_COUNT);
  Program best = current;
  Score best_score = current_score;

  printf("seed=%d operations=%d iterations=%d\n", seed, op_count, iterations);
  print_program(&best, &best_score);
  printf("validation all valid: %s\n", validate_all(&best) ? "yes" : "no");

  double temperature = 20000.0;
  for (int i = 1; i <= iterations; i++) {
    Program mutated;
    mutate_program(&mutated, &current, ops, op_count);
    Score mutated_score = score_program(&mutated, TRAIN_TESTS, TRAIN_COUNT);

    double delta = mutated_score.cost - current_score.cost;
    bool accept = delta <= 0.0 || rng_double() < exp(-delta / temperature);
    if (accept) {
      current = mutated;
      current_score = mutated_score;
    }

    if (mutated_score.cost < best_score.cost) {
      best = mutated;
      best_score = mutated_score;
      printf("\nnew best at iteration %d, temperature=%.6g\n", i, temperature);
      print_program(&best, &best_score);
      if (best_score.exact == best_score.total && best_score.penalty == 0.0) {
        printf("validation all valid: %s\n", validate_all(&best) ? "yes" : "no");
      }
      fflush(stdout);
    }

    temperature *= 0.99998;
    if (temperature < 0.01) temperature = 0.01;
  }

  printf("\nfinal best:\n");
  print_program(&best, &best_score);
  printf("validation all valid: %s\n", validate_all(&best) ? "yes" : "no");
  return 0;
}
