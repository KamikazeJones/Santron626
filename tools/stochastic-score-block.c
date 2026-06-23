#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SANTRON_VM_NO_MAIN
#include "santron-vm.c"

#define MAX_CELLS 48
#define MAX_OPS 256
#define MAX_PROGRAM_OPS 48
#define START_PC 30
#define INPUT_PC_AFTER_RS 1

typedef struct {
  int m7;
  int pos;
  int black;
  int hit;
} TestCase;

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
  double error;
  double penalty;
  double cost;
  int cells;
} Score;

typedef enum {
  VARIANT_12 = 12,
  VARIANT_13 = 13,
  VARIANT_16 = 16
} TargetVariant;

static const TestCase TRAIN_TESTS[] = {
  {0, 1, 0, 4}, {1, 1, 0, 4}, {2, 1, 1, 3}, {4, 1, 2, 2},
  {0, 2, 0, 4}, {1, 2, 0, 4}, {2, 2, 1, 3}, {4, 2, 2, 2},
  {0, 3, 0, 4}, {1, 3, 0, 4}, {3, 3, 1, 3}, {4, 3, 2, 2},
  {0, 4, 0, 4}, {1, 4, 0, 4}, {3, 4, 1, 3}, {4, 4, 2, 2},
};

static const int TRAIN_COUNT = (int)(sizeof(TRAIN_TESTS) / sizeof(TRAIN_TESTS[0]));

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
  s->program[0] = KEY_RS;
  for (int i = 0; i < len && START_PC + i < PROGRAM_SIZE; i++) {
    s->program[START_PC + i] = (int)codes[i];
  }
  s->pc = START_PC;
}

static void expected_state(const TestCase *test, TargetVariant variant, int *black, int *m9, int *pos, bool *paused) {
  int is_hit = test->m7 != 0;
  int is_black = test->m7 == test->pos;
  *black = test->black + is_black;
  *m9 = test->hit - (is_hit ? 0 : 1);
  if ((variant == VARIANT_13 || variant == VARIANT_16) && is_black) *m9 -= 1;
  *pos = test->pos + 1;
  *paused = *pos == 5;
}

static Score score_program(const Program *p, const TestCase *tests, int test_count, TargetVariant variant) {
  uint16_t codes[MAX_CELLS];
  int len = flatten_program(p, codes, MAX_CELLS);
  Score score = {0};
  score.cells = len;

  for (int i = 0; i < test_count; i++) {
    int expected_black = 0;
    int expected_m9 = 0;
    int expected_pos = 0;
    bool expected_paused = false;
    expected_state(&tests[i], variant, &expected_black, &expected_m9, &expected_pos, &expected_paused);

    SantronState s;
    init_state(&s);
    load_candidate(&s, codes, len);
    s.memory[0] = 1.0;
    s.memory[1] = (double)tests[i].black;
    s.memory[2] = (double)tests[i].pos;
    s.memory[4] = 9.0;
    s.memory[7] = (double)tests[i].m7;
    s.memory[8] = 3040210.0;
    s.memory[9] = (double)tests[i].hit;

    run_program(&s, 200);

    bool ok = true;
    ok = ok && nearly_equal(s.memory[0], 1.0);
    ok = ok && nearly_equal(s.memory[1], (double)expected_black);
    ok = ok && nearly_equal(s.memory[2], (double)expected_pos);
    ok = ok && nearly_equal(s.memory[4], 9.0);
    ok = ok && nearly_equal(s.memory[7], (double)tests[i].m7);
    ok = ok && nearly_equal(s.memory[8], 3040210.0);
    ok = ok && nearly_equal(s.memory[9], (double)expected_m9);
    ok = ok && s.pending == OP_NONE;
    ok = ok && s.paren_depth == 0;
    ok = ok && s.pending_memory < 0;
    ok = ok && !s.exponent_entry;
    ok = ok && s.paused;
    ok = ok && (expected_paused ? (s.pc != INPUT_PC_AFTER_RS) : (s.pc == INPUT_PC_AFTER_RS));
    if (variant == VARIANT_13 && expected_paused) {
      ok = ok && nearly_equal(s.x, (double)expected_black);
      SantronState resumed = s;
      run_program(&resumed, 20);
      ok = ok && resumed.paused;
      ok = ok && nearly_equal(resumed.x, (double)expected_m9);
    } else if (variant == VARIANT_16 && expected_paused) {
      for (int x = 0; x <= 4; x++) {
        SantronState resumed = s;
        set_x(&resumed, (double)x);
        run_program(&resumed, 50);
        ok = ok && resumed.paused;
        ok = ok && resumed.pc == INPUT_PC_AFTER_RS;
        ok = ok && nearly_equal(resumed.memory[1], 0.0);
        ok = ok && nearly_equal(resumed.memory[2], 1.0);
        ok = ok && nearly_equal(resumed.memory[9], 4.0);
      }
    }
    if (ok) score.exact++;

    score.error += fabs(s.memory[1] - (double)expected_black);
    score.error += fabs(s.memory[2] - (double)expected_pos);
    score.error += fabs(s.memory[9] - (double)expected_m9);
    if (variant == VARIANT_13 && expected_paused) score.error += fabs(s.x - (double)expected_black);
    if (variant == VARIANT_16 && expected_paused) {
      for (int x = 0; x <= 4; x++) {
        SantronState resumed = s;
        set_x(&resumed, (double)x);
        run_program(&resumed, 50);
        score.error += fabs(resumed.memory[1]);
        score.error += fabs(resumed.memory[2] - 1.0);
        score.error += fabs(resumed.memory[9] - 4.0);
        if (!resumed.paused) score.penalty += 1000.0;
        score.error += fabs((double)(resumed.pc - INPUT_PC_AFTER_RS));
      }
    }
    if (!s.paused) score.penalty += 1000.0;
    if (!expected_paused) score.error += fabs((double)(s.pc - INPUT_PC_AFTER_RS));
    else if (s.pc == INPUT_PC_AFTER_RS) score.penalty += 1000.0;

    if (!nearly_equal(s.memory[0], 1.0)) score.penalty += 1000.0;
    if (!nearly_equal(s.memory[4], 9.0)) score.penalty += 1000.0;
    if (!nearly_equal(s.memory[7], (double)tests[i].m7)) score.penalty += 1000.0;
    if (!nearly_equal(s.memory[8], 3040210.0)) score.penalty += 1000.0;
    if (s.pending != OP_NONE) score.penalty += 1000.0;
    if (s.paren_depth != 0) score.penalty += 1000.0;
    if (s.pending_memory >= 0) score.penalty += 100.0;
    if (s.exponent_entry) score.penalty += 100.0;
  }

  score.cost =
    (double)(test_count - score.exact) * 100000.0 +
    score.error * 100.0 +
    score.penalty +
    (double)score.cells;
  return score;
}

static bool validate_all(const Program *p, TargetVariant variant) {
  uint16_t codes[MAX_CELLS];
  int len = flatten_program(p, codes, MAX_CELLS);
  for (int m7 = 0; m7 <= 4; m7++) {
    for (int pos = 1; pos <= 4; pos++) {
      for (int black0 = 0; black0 <= 3; black0++) {
        for (int hit0 = 0; hit0 <= 4; hit0++) {
          TestCase test = {m7, pos, black0, hit0};
          int expected_black, expected_m9, expected_pos;
          bool expected_paused;
          expected_state(&test, variant, &expected_black, &expected_m9, &expected_pos, &expected_paused);

          SantronState s;
          init_state(&s);
          load_candidate(&s, codes, len);
          s.memory[0] = 1.0;
          s.memory[1] = (double)black0;
          s.memory[2] = (double)pos;
          s.memory[4] = 9.0;
          s.memory[7] = (double)m7;
          s.memory[8] = 3040210.0;
          s.memory[9] = (double)hit0;
          run_program(&s, 200);

          if (!nearly_equal(s.memory[1], (double)expected_black)) return false;
          if (!nearly_equal(s.memory[2], (double)expected_pos)) return false;
          if (!nearly_equal(s.memory[9], (double)expected_m9)) return false;
          if (!s.paused) return false;
          if (!expected_paused && s.pc != INPUT_PC_AFTER_RS) return false;
          if (expected_paused && s.pc == INPUT_PC_AFTER_RS) return false;
          if (variant == VARIANT_13 && expected_paused) {
            if (!nearly_equal(s.x, (double)expected_black)) return false;
            SantronState resumed = s;
            run_program(&resumed, 20);
            if (!resumed.paused) return false;
            if (!nearly_equal(resumed.x, (double)expected_m9)) return false;
          } else if (variant == VARIANT_16 && expected_paused) {
            for (int x = 0; x <= 4; x++) {
              SantronState resumed = s;
              set_x(&resumed, (double)x);
              run_program(&resumed, 50);
              if (!resumed.paused || resumed.pc != INPUT_PC_AFTER_RS) return false;
              if (!nearly_equal(resumed.memory[1], 0.0)) return false;
              if (!nearly_equal(resumed.memory[2], 1.0)) return false;
              if (!nearly_equal(resumed.memory[9], 4.0)) return false;
            }
          }
          if (s.pending != OP_NONE || s.paren_depth != 0 || s.pending_memory >= 0 || s.exponent_entry) return false;
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
    KEY_EQ, KEY_LPAREN, KEY_RPAREN, KEY_RS, KEY_SKP
  };
  const int memory_commands[] = {
    KEY_MPLUS, KEY_MMINUS, KEY_MULMEM, KEY_DIVMEM, KEY_STO, KEY_RCL
  };

  int count = 0;
  for (size_t i = 0; i < sizeof(single_cell) / sizeof(single_cell[0]); i++) {
    ops[count].codes[0] = (uint16_t)single_cell[i];
    ops[count].len = 1;
    count++;
  }
  for (int digit = 0; digit <= 9; digit++) {
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
  ops[count++] = (Operation){{KEY_GOTO, KEY_DIGIT0 + 0, KEY_DIGIT0 + 0}, 3};
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
    case KEY_GOTO: return "GOTO";
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
         len, p->op_count, score->exact, TRAIN_COUNT, score->error,
         score->penalty, score->cost);
  for (int i = 0; i < len; i++) printf("%s%s", i ? " " : "", code_name(codes[i]));
  printf("\n");
  for (int i = 0; i < len; i++) printf("%02d %03d %s\n", i, codes[i], code_name(codes[i]));
}

static void seed_score_block(Program *p) {
  memset(p, 0, sizeof(*p));
  const Operation seed[] = {
    {{KEY_RCL, KEY_DIGIT0 + 7, 0}, 2},
    {{KEY_SQR, 0, 0}, 1},
    {{KEY_SIGN, 0, 0}, 1},
    {{KEY_SKP, 0, 0}, 1},
    {{KEY_RCL, KEY_DIGIT0 + 0, 0}, 2},
    {{KEY_MMINUS, KEY_DIGIT0 + 9, 0}, 2},
    {{KEY_RCL, KEY_DIGIT0 + 7, 0}, 2},
    {{KEY_SUB, 0, 0}, 1},
    {{KEY_RCL, KEY_DIGIT0 + 2, 0}, 2},
    {{KEY_EQ, 0, 0}, 1},
    {{KEY_SQR, 0, 0}, 1},
    {{KEY_SIGN, 0, 0}, 1},
    {{KEY_SKP, 0, 0}, 1},
    {{KEY_RCL, KEY_DIGIT0 + 0, 0}, 2},
    {{KEY_MPLUS, KEY_DIGIT0 + 1, 0}, 2},
    {{KEY_DIGIT0 + 1, 0, 0}, 1},
    {{KEY_MPLUS, KEY_DIGIT0 + 2, 0}, 2},
    {{KEY_RCL, KEY_DIGIT0 + 2, 0}, 2},
    {{KEY_SUB, 0, 0}, 1},
    {{KEY_DIGIT0 + 5, 0, 0}, 1},
    {{KEY_EQ, 0, 0}, 1},
    {{KEY_SKP, 0, 0}, 1},
    {{KEY_RS, 0, 0}, 1},
    {{KEY_GOTO, KEY_DIGIT0 + 0, KEY_DIGIT0 + 0}, 3},
  };
  p->op_count = (int)(sizeof(seed) / sizeof(seed[0]));
  memcpy(p->ops, seed, sizeof(seed));
}

static void seed_score_block_13(Program *p) {
  memset(p, 0, sizeof(*p));
  const Operation seed[] = {
    {{KEY_RCL, KEY_DIGIT0 + 7, 0}, 2},
    {{KEY_SQR, 0, 0}, 1},
    {{KEY_SIGN, 0, 0}, 1},
    {{KEY_SKP, 0, 0}, 1},
    {{KEY_RCL, KEY_DIGIT0 + 0, 0}, 2},
    {{KEY_MMINUS, KEY_DIGIT0 + 9, 0}, 2},
    {{KEY_RCL, KEY_DIGIT0 + 7, 0}, 2},
    {{KEY_SUB, 0, 0}, 1},
    {{KEY_RCL, KEY_DIGIT0 + 2, 0}, 2},
    {{KEY_EQ, 0, 0}, 1},
    {{KEY_SQR, 0, 0}, 1},
    {{KEY_SIGN, 0, 0}, 1},
    {{KEY_SKP, 0, 0}, 1},
    {{KEY_RCL, KEY_DIGIT0 + 0, 0}, 2},
    {{KEY_MPLUS, KEY_DIGIT0 + 1, 0}, 2},
    {{KEY_MMINUS, KEY_DIGIT0 + 9, 0}, 2},
    {{KEY_DIGIT0 + 1, 0, 0}, 1},
    {{KEY_MPLUS, KEY_DIGIT0 + 2, 0}, 2},
    {{KEY_DIGIT0 + 4, 0, 0}, 1},
    {{KEY_SUB, 0, 0}, 1},
    {{KEY_RCL, KEY_DIGIT0 + 2, 0}, 2},
    {{KEY_EQ, 0, 0}, 1},
    {{KEY_SKP, 0, 0}, 1},
    {{KEY_GOTO, KEY_DIGIT0 + 0, KEY_DIGIT0 + 0}, 3},
    {{KEY_RCL, KEY_DIGIT0 + 1, 0}, 2},
    {{KEY_RS, 0, 0}, 1},
    {{KEY_RCL, KEY_DIGIT0 + 9, 0}, 2},
    {{KEY_RS, 0, 0}, 1},
  };
  p->op_count = (int)(sizeof(seed) / sizeof(seed[0]));
  memcpy(p->ops, seed, sizeof(seed));
}

static void seed_score_block_16(Program *p) {
  memset(p, 0, sizeof(*p));
  const Operation seed[] = {
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
    {{KEY_RCL, KEY_DIGIT0 + 2, 0}, 2},
    {{KEY_EQ, 0, 0}, 1},
    {{KEY_SKP, 0, 0}, 1},
    {{KEY_GOTO, KEY_DIGIT0 + 0, KEY_DIGIT0 + 0}, 3},
    {{KEY_RS, 0, 0}, 1},
    {{KEY_CLEAR, 0, 0}, 1},
    {{KEY_STO, KEY_DIGIT0 + 1, 0}, 2},
    {{KEY_DIGIT0 + 4, 0, 0}, 1},
    {{KEY_STO, KEY_DIGIT0 + 9, 0}, 2},
    {{KEY_MMINUS, KEY_DIGIT0 + 2, 0}, 2},
    {{KEY_GOTO, KEY_DIGIT0 + 0, KEY_DIGIT0 + 0}, 3},
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

int main(int argc, char **argv) {
  const int iterations = parse_int_arg(argc, argv, "--iterations", 300000);
  const int seed = parse_int_arg(argc, argv, "--seed", (int)time(NULL));
  const int variant_arg = parse_int_arg(argc, argv, "--variant", 13);
  TargetVariant variant = VARIANT_13;
  if (variant_arg == 12) variant = VARIANT_12;
  else if (variant_arg == 16) variant = VARIANT_16;
  rng_state = (uint64_t)(unsigned int)seed;
  if (!rng_state) rng_state = 1;

  Operation ops[MAX_OPS];
  int op_count = full_operation_set(ops, MAX_OPS);

  Program current;
  if (variant == VARIANT_12) seed_score_block(&current);
  else if (variant == VARIANT_16) seed_score_block_16(&current);
  else seed_score_block_13(&current);
  Score current_score = score_program(&current, TRAIN_TESTS, TRAIN_COUNT, variant);
  Program best = current;
  Score best_score = current_score;

  printf("seed=%d variant=%d operations=%d iterations=%d\n", seed, (int)variant, op_count, iterations);
  print_program(&best, &best_score);
  printf("validation all valid: %s\n", validate_all(&best, variant) ? "yes" : "no");

  double temperature = 20000.0;
  for (int i = 1; i <= iterations; i++) {
    Program mutated;
    mutate_program(&mutated, &current, ops, op_count);
    Score mutated_score = score_program(&mutated, TRAIN_TESTS, TRAIN_COUNT, variant);

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
      if (best_score.exact == TRAIN_COUNT && best_score.penalty == 0.0) {
        printf("validation all valid: %s\n", validate_all(&best, variant) ? "yes" : "no");
      }
      fflush(stdout);
    }

    temperature *= 0.99998;
    if (temperature < 0.01) temperature = 0.01;
  }

  printf("\nfinal best:\n");
  print_program(&best, &best_score);
  printf("validation all valid: %s\n", validate_all(&best, variant) ? "yes" : "no");
  return 0;
}
