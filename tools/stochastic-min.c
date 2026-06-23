#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SANTRON_VM_NO_MAIN
#include "santron-vm.c"

#define MAX_CELLS 32
#define MAX_OPS 256
#define MAX_PROGRAM_OPS 32

typedef struct {
  double a;
  double b;
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

static const TestCase TRAIN_TESTS[] = {
  {0, 0}, {0, 1}, {1, 0}, {2, 5}, {5, 2}, {7, 7},
  {10, 4}, {4, 10}, {-2, 3}, {3, -2}, {-5, -2}, {-2, -5},
  {9, 1}, {1, 9}, {-9, 4}, {4, -9}, {-7, -8}, {-8, -7}
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
  for (int i = 0; i < len && i < PROGRAM_SIZE; i++) s->program[i] = (int)codes[i];
  if (len < PROGRAM_SIZE) s->program[len] = KEY_RS;
  s->pc = 0;
}

static Score score_program(const Program *p, const TestCase *tests, int test_count) {
  uint16_t codes[MAX_CELLS];
  int len = flatten_program(p, codes, MAX_CELLS);
  Score score = {0};
  score.cells = len;

  for (int i = 0; i < test_count; i++) {
    const double a = tests[i].a;
    const double b = tests[i].b;
    const double target = fmin(a, b);

    SantronState s;
    init_state(&s);
    load_candidate(&s, codes, len);
    s.memory[0] = a;
    s.memory[1] = b;
    run_program(&s, 300);

    if (nearly_equal(s.x, target) &&
        nearly_equal(s.memory[0], a) &&
        nearly_equal(s.memory[1], b) &&
        s.pending == OP_NONE &&
        s.paren_depth == 0 &&
        s.pending_memory < 0 &&
        !s.exponent_entry) {
      score.exact++;
    }

    if (isfinite(s.x)) score.error += fabs(s.x - target);
    else score.error += 1e6;

    if (!nearly_equal(s.memory[0], a)) score.penalty += 1000.0;
    if (!nearly_equal(s.memory[1], b)) score.penalty += 1000.0;
    if (s.pending != OP_NONE) score.penalty += 1000.0;
    if (s.paren_depth != 0) score.penalty += 1000.0;
    if (s.pending_memory >= 0) score.penalty += 100.0;
    if (s.exponent_entry) score.penalty += 100.0;
    if (s.running) score.penalty += 100.0;
  }

  score.cost =
    (double)(test_count - score.exact) * 100000.0 +
    score.error * 10.0 +
    score.penalty +
    (double)score.cells;
  return score;
}

static bool validate_grid(const Program *p, int min_value, int max_value) {
  uint16_t codes[MAX_CELLS];
  int len = flatten_program(p, codes, MAX_CELLS);

  for (int a = min_value; a <= max_value; a++) {
    for (int b = min_value; b <= max_value; b++) {
      SantronState s;
      init_state(&s);
      load_candidate(&s, codes, len);
      s.memory[0] = (double)a;
      s.memory[1] = (double)b;
      run_program(&s, 300);
      if (!nearly_equal(s.x, fmin((double)a, (double)b))) return false;
      if (!nearly_equal(s.memory[0], (double)a)) return false;
      if (!nearly_equal(s.memory[1], (double)b)) return false;
      if (s.pending != OP_NONE || s.paren_depth != 0) return false;
      if (s.pending_memory >= 0 || s.exponent_entry) return false;
    }
  }
  return true;
}

static int full_operation_set(Operation *ops, int capacity) {
  const int single_cell[] = {
    KEY_ADD, KEY_SUB, KEY_MUL, KEY_DIV, KEY_YX, KEY_INV, KEY_SQR, KEY_SQRT,
    KEY_SIN, KEY_COS, KEY_TAN, KEY_ASIN, KEY_ACOS, KEY_ATAN,
    KEY_LN, KEY_LOG, KEY_EX, KEY_TENX,
    KEY_CLEAR, KEY_SIGN, KEY_EXP, KEY_SWAP, KEY_PI, KEY_DOT, KEY_PS,
    KEY_EQ, KEY_LPAREN, KEY_RPAREN, KEY_RS, KEY_SKP
  };
  const int memory_commands[] = {
    KEY_MPLUS, KEY_MMINUS, KEY_MULMEM, KEY_DIVMEM, KEY_STO, KEY_RCL
  };

  int count = 0;
  for (size_t i = 0; i < sizeof(single_cell) / sizeof(single_cell[0]); i++) {
    if (count < capacity) {
      ops[count].codes[0] = (uint16_t)single_cell[i];
      ops[count].len = 1;
      count++;
    }
  }
  for (int digit = 0; digit <= 9; digit++) {
    if (count < capacity) {
      ops[count].codes[0] = (uint16_t)(KEY_DIGIT0 + digit);
      ops[count].len = 1;
      count++;
    }
  }
  for (size_t i = 0; i < sizeof(memory_commands) / sizeof(memory_commands[0]); i++) {
    for (int digit = 0; digit <= 9; digit++) {
      if (count < capacity) {
        ops[count].codes[0] = (uint16_t)memory_commands[i];
        ops[count].codes[1] = (uint16_t)(KEY_DIGIT0 + digit);
        ops[count].len = 2;
        count++;
      }
    }
  }
  for (int target = 0; target < PROGRAM_SIZE; target++) {
    if (count < capacity) {
      ops[count].codes[0] = KEY_GOTO;
      ops[count].codes[1] = (uint16_t)(KEY_DIGIT0 + target / 10);
      ops[count].codes[2] = (uint16_t)(KEY_DIGIT0 + target % 10);
      ops[count].len = 3;
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
    case KEY_YX: return "Y^X";
    case KEY_INV: return "1/X";
    case KEY_SQR: return "X^2";
    case KEY_SQRT: return "SQRT";
    case KEY_SIN: return "SIN";
    case KEY_COS: return "COS";
    case KEY_TAN: return "TAN";
    case KEY_ASIN: return "ASIN";
    case KEY_ACOS: return "ACOS";
    case KEY_ATAN: return "ATAN";
    case KEY_LN: return "LN";
    case KEY_LOG: return "LOG";
    case KEY_EX: return "E^X";
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
    case KEY_PS: return "PS";
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
  for (int i = 0; i < len; i++) {
    printf("%s%s", i ? " " : "", code_name(codes[i]));
  }
  printf("\n");
  for (int i = 0; i < len; i++) {
    printf("%02d %03d %s\n", i, codes[i], code_name(codes[i]));
  }
}

static void seed_known_min(Program *p) {
  memset(p, 0, sizeof(*p));
  const Operation seed[] = {
    {{KEY_RCL, KEY_DIGIT0 + 0, 0}, 2},
    {{KEY_SUB, 0, 0}, 1},
    {{KEY_RCL, KEY_DIGIT0 + 1, 0}, 2},
    {{KEY_STO, KEY_DIGIT0 + 2, 0}, 2},
    {{KEY_EQ, 0, 0}, 1},
    {{KEY_SKP, 0, 0}, 1},
    {{KEY_CLEAR, 0, 0}, 1},
    {{KEY_ADD, 0, 0}, 1},
    {{KEY_RCL, KEY_DIGIT0 + 2, 0}, 2},
    {{KEY_EQ, 0, 0}, 1},
  };
  p->op_count = (int)(sizeof(seed) / sizeof(seed[0]));
  memcpy(p->ops, seed, sizeof(seed));
}

static void mutate_program(Program *dst, const Program *src, const Operation *ops, int op_count) {
  *dst = *src;
  int mutation = rng_int(6);

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
  } else if (dst->op_count > 0) {
    int idx = rng_int(dst->op_count);
    if (dst->ops[idx].len == 2) {
      dst->ops[idx].codes[1] = (uint16_t)(KEY_DIGIT0 + rng_int(10));
    } else if (dst->ops[idx].len == 3 && dst->ops[idx].codes[0] == KEY_GOTO) {
      int target = rng_int(PROGRAM_SIZE);
      dst->ops[idx].codes[1] = (uint16_t)(KEY_DIGIT0 + target / 10);
      dst->ops[idx].codes[2] = (uint16_t)(KEY_DIGIT0 + target % 10);
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
  const int iterations = parse_int_arg(argc, argv, "--iterations", 200000);
  const int seed = parse_int_arg(argc, argv, "--seed", (int)time(NULL));
  rng_state = (uint64_t)(unsigned int)seed;
  if (rng_state == 0) rng_state = 1;

  Operation ops[MAX_OPS];
  int op_count = full_operation_set(ops, MAX_OPS);

  Program current;
  seed_known_min(&current);
  Score current_score = score_program(&current, TRAIN_TESTS, TRAIN_COUNT);
  Program best = current;
  Score best_score = current_score;

  printf("seed=%d operations=%d iterations=%d\n", seed, op_count, iterations);
  print_program(&best, &best_score);
  printf("grid -9..9 valid: %s\n", validate_grid(&best, -9, 9) ? "yes" : "no");

  double temperature = 10000.0;
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

    bool better = mutated_score.cost < best_score.cost;

    if (better) {
      best = mutated;
      best_score = mutated_score;
      printf("\nnew best at iteration %d, temperature=%.6g\n", i, temperature);
      print_program(&best, &best_score);
      if (best_score.exact == TRAIN_COUNT) {
        printf("grid -9..9 valid: %s\n", validate_grid(&best, -9, 9) ? "yes" : "no");
      }
      fflush(stdout);
    }

    temperature *= 0.99995;
    if (temperature < 0.01) temperature = 0.01;
  }

  printf("\nfinal best:\n");
  print_program(&best, &best_score);
  printf("grid -9..9 valid: %s\n", validate_grid(&best, -9, 9) ? "yes" : "no");
  return 0;
}
