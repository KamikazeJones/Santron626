#include <float.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SANTRON_VM_NO_MAIN
#include "santron-vm.c"

#define MAX_CELLS 32
#define DEFAULT_MAX_CELLS 14
#define DEFAULT_BEAM 5000
#define MAX_OPS 256

typedef struct {
  double a;
  double b;
} TestCase;

typedef struct {
  uint16_t codes[MAX_CELLS];
  int len;
  int exact;
  int useful;
  double error;
  double penalty;
} Candidate;

typedef struct {
  uint16_t codes[3];
  int len;
} Operation;

static const TestCase TESTS[] = {
  {0, 0}, {0, 1}, {1, 0}, {2, 5}, {5, 2}, {7, 7},
  {10, 4}, {4, 10}, {-2, 3}, {3, -2}, {-5, -2}, {-2, -5}
};

static const int TEST_COUNT = (int)(sizeof(TESTS) / sizeof(TESTS[0]));

static bool nearly_equal(double a, double b) {
  if (isnan(a) && isnan(b)) return true;
  return fabs(a - b) <= 1e-8;
}

static void load_candidate(SantronState *s, const uint16_t *codes, int len) {
  for (int i = 0; i < PROGRAM_SIZE; i++) s->program[i] = KEY_BLANK;
  for (int i = 0; i < len && i < PROGRAM_SIZE; i++) s->program[i] = (int)codes[i];
  if (len < PROGRAM_SIZE) s->program[len] = KEY_RS;
  s->pc = 0;
}

static double clipped_error(double x, double target) {
  if (!isfinite(x)) return 1e9;
  double e = fabs(x - target);
  return e > 1e9 ? 1e9 : e;
}

static void score_candidate(Candidate *candidate) {
  candidate->exact = 0;
  candidate->useful = 0;
  candidate->error = 0.0;
  candidate->penalty = 0.0;

  double values[4][TEST_COUNT];

  for (int i = 0; i < TEST_COUNT; i++) {
    const double a = TESTS[i].a;
    const double b = TESTS[i].b;
    const double target = fmin(a, b);

    SantronState s;
    init_state(&s);
    load_candidate(&s, candidate->codes, candidate->len);
    s.memory[0] = a;
    s.memory[1] = b;

    run_program(&s, 300);

    values[0][i] = s.x;
    values[1][i] = s.y;
    values[2][i] = s.memory[2];
    values[3][i] = s.memory[3];

    if (nearly_equal(s.x, target) &&
        nearly_equal(s.memory[0], a) &&
        nearly_equal(s.memory[1], b) &&
        s.pending_memory < 0 &&
        !s.exponent_entry) {
      candidate->exact++;
    }

    candidate->error += clipped_error(s.x, target);
    if (!nearly_equal(s.memory[0], a)) candidate->penalty += 100.0;
    if (!nearly_equal(s.memory[1], b)) candidate->penalty += 100.0;
    if (s.pending_memory >= 0) candidate->penalty += 10.0;
    if (s.exponent_entry) candidate->penalty += 10.0;
    if (s.running) candidate->penalty += 10.0;
  }

  bool relation_seen[6] = {false, false, false, false, false, false};
  for (int relation = 0; relation < 6; relation++) {
    for (int value_idx = 0; value_idx < 4; value_idx++) {
      bool matches = true;
      for (int i = 0; i < TEST_COUNT; i++) {
        const double a = TESTS[i].a;
        const double b = TESTS[i].b;
        double expected = 0.0;
        if (relation == 0) expected = a;
        else if (relation == 1) expected = b;
        else if (relation == 2) expected = a - b;
        else if (relation == 3) expected = b - a;
        else if (relation == 4) expected = a + b;
        else if (relation == 5) expected = fmin(a, b);
        if (!nearly_equal(values[value_idx][i], expected)) {
          matches = false;
          break;
        }
      }
      if (matches) {
        relation_seen[relation] = true;
        break;
      }
    }
  }
  for (int relation = 0; relation < 6; relation++) {
    if (relation_seen[relation]) candidate->useful += 10;
  }
}

static bool is_solution(const Candidate *candidate) {
  return candidate->exact == TEST_COUNT && candidate->penalty == 0.0;
}

static int compare_candidates(const void *left_ptr, const void *right_ptr) {
  const Candidate *left = (const Candidate *)left_ptr;
  const Candidate *right = (const Candidate *)right_ptr;

  if (left->penalty < right->penalty) return -1;
  if (left->penalty > right->penalty) return 1;
  if (left->useful != right->useful) return right->useful - left->useful;
  if (left->exact != right->exact) return right->exact - left->exact;
  if (left->error < right->error) return -1;
  if (left->error > right->error) return 1;
  return left->len - right->len;
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
    default:
      break;
  }
  static char digit_names[10][2] = {
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9"
  };
  int digit = digit_from_code(code);
  if (digit >= 0) return digit_names[digit];
  return "?";
}

static void print_candidate(const Candidate *candidate) {
  printf("%d cells, exact=%d/%d useful=%d error=%.12g penalty=%.12g\n",
         candidate->len, candidate->exact, TEST_COUNT, candidate->useful,
         candidate->error, candidate->penalty);
  for (int i = 0; i < candidate->len; i++) {
    printf("%s%s", i ? " " : "", code_name(candidate->codes[i]));
  }
  printf("\n");
  for (int i = 0; i < candidate->len; i++) {
    printf("%02d %03d %s\n", i, candidate->codes[i], code_name(candidate->codes[i]));
  }
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

static int parse_int_arg(int argc, char **argv, const char *name, int default_value) {
  for (int i = 1; i + 1 < argc; i++) {
    if (strcmp(argv[i], name) == 0) return atoi(argv[i + 1]);
  }
  return default_value;
}

static bool has_arg(int argc, char **argv, const char *name) {
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], name) == 0) return true;
  }
  return false;
}

static void verify_known_min(void) {
  Candidate known = {0};
  const uint16_t codes[] = {
    KEY_RCL, KEY_DIGIT0 + 0,
    KEY_SUB,
    KEY_RCL, KEY_DIGIT0 + 1,
    KEY_STO, KEY_DIGIT0 + 2,
    KEY_EQ,
    KEY_SKP,
    KEY_CLEAR,
    KEY_ADD,
    KEY_RCL, KEY_DIGIT0 + 2,
    KEY_EQ
  };
  known.len = (int)(sizeof(codes) / sizeof(codes[0]));
  memcpy(known.codes, codes, sizeof(codes));
  score_candidate(&known);
  printf("known routine:\n");
  print_candidate(&known);
}

int main(int argc, char **argv) {
  const int max_cells = parse_int_arg(argc, argv, "--max-cells", DEFAULT_MAX_CELLS);
  const int beam_size = parse_int_arg(argc, argv, "--beam", DEFAULT_BEAM);
  const bool show_known = has_arg(argc, argv, "--known");

  if (max_cells < 1 || max_cells > MAX_CELLS || beam_size < 1) {
    fprintf(stderr, "Usage: %s [--max-cells n] [--beam n] [--known]\n", argv[0]);
    return 2;
  }

  if (show_known) verify_known_min();

  Operation ops[MAX_OPS];
  const int op_count = full_operation_set(ops, MAX_OPS);

  Candidate *beam = calloc((size_t)beam_size, sizeof(Candidate));
  Candidate *next = calloc((size_t)beam_size * (size_t)op_count, sizeof(Candidate));
  if (!beam || !next) {
    fprintf(stderr, "out of memory\n");
    return 1;
  }

  int beam_count = 1;
  score_candidate(&beam[0]);

  for (int round = 1; round <= max_cells; round++) {
    int next_count = 0;
    for (int i = 0; i < beam_count; i++) {
      for (int k = 0; k < op_count; k++) {
        Candidate c = beam[i];
        if (c.len + ops[k].len > max_cells || c.len + ops[k].len > MAX_CELLS) continue;
        for (int j = 0; j < ops[k].len; j++) c.codes[c.len++] = ops[k].codes[j];
        score_candidate(&c);
        next[next_count++] = c;
      }
    }

    if (next_count == 0) break;
    qsort(next, (size_t)next_count, sizeof(Candidate), compare_candidates);

    int copy_count = next_count < beam_size ? next_count : beam_size;
    memcpy(beam, next, (size_t)copy_count * sizeof(Candidate));
    beam_count = copy_count;

    printf("round %02d best: ", round);
    print_candidate(&beam[0]);

    for (int i = 0; i < beam_count; i++) {
      if (is_solution(&beam[i])) {
        printf("FOUND solution at round %d, rank %d:\n", round, i);
        print_candidate(&beam[i]);
        free(beam);
        free(next);
        return 0;
      }
    }
  }

  printf("No solution found up to %d cells. Best candidate:\n", max_cells);
  print_candidate(&beam[0]);
  free(beam);
  free(next);
  return 1;
}
