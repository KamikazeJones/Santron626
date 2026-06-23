#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SANTRON_VM_NO_MAIN
#include "santron-vm.c"

#define MAX_CELLS 6
#define MAX_OPS 128
#define SIGNATURE_SIZE 4096
#define HASH_SIZE 262144
#define DEFAULT_SAMPLES 50000

typedef struct {
  uint16_t codes[3];
  int len;
} Operation;

typedef struct {
  uint16_t codes[MAX_CELLS];
  int len;
} Sequence;

typedef struct {
  char *signature;
  Sequence best;
} Entry;

static uint64_t rng_state = 0x123456789abcdef0ULL;
static Entry table[HASH_SIZE];

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

static uint64_t hash_string(const char *text) {
  uint64_t h = 1469598103934665603ULL;
  while (*text) {
    h ^= (unsigned char)*text++;
    h *= 1099511628211ULL;
  }
  return h;
}

static char *copy_string(const char *text) {
  size_t len = strlen(text) + 1;
  char *copy = malloc(len);
  if (!copy) {
    fprintf(stderr, "out of memory\n");
    exit(1);
  }
  memcpy(copy, text, len);
  return copy;
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

static void load_sequence(SantronState *s, const Sequence *seq) {
  for (int i = 0; i < PROGRAM_SIZE; i++) s->program[i] = KEY_BLANK;
  for (int i = 0; i < seq->len; i++) s->program[i] = seq->codes[i];
  s->program[seq->len] = KEY_RS;
  s->pc = 0;
}

static void append_number(char *out, size_t out_size, double value) {
  char buf[64];
  if (isnan(value)) snprintf(buf, sizeof(buf), "NaN");
  else if (isinf(value)) snprintf(buf, sizeof(buf), "%sInf", value < 0 ? "-" : "");
  else snprintf(buf, sizeof(buf), "%.12g", value);
  strncat(out, buf, out_size - strlen(out) - 1);
}

static void append_state_signature(char *out, size_t out_size, const SantronState *s) {
  append_number(out, out_size, s->x);
  strncat(out, ",", out_size - strlen(out) - 1);
  append_number(out, out_size, s->y);
  char buf[128];
  snprintf(buf, sizeof(buf), ",p%d,e%d,d%d,pm%d,pe%d,pd%d,depth%d",
           (int)s->pending, s->entering ? 1 : 0, s->decimal_entry ? 1 : 0,
           s->pending_memory, s->exponent_entry ? 1 : 0,
           s->pending_precision ? 1 : 0, s->paren_depth);
  strncat(out, buf, out_size - strlen(out) - 1);
  for (int i = 0; i < MEMORY_SIZE; i++) {
    strncat(out, ",M", out_size - strlen(out) - 1);
    snprintf(buf, sizeof(buf), "%d=", i);
    strncat(out, buf, out_size - strlen(out) - 1);
    append_number(out, out_size, s->memory[i]);
  }
}

static void setup_case(SantronState *s, int case_index) {
  init_state(s);
  const double xs[] = {0, 1, -1, 2.5, -3.75, 1000, 0.125, 9};
  const double ys[] = {0, 2, -4, 10, -8, 0.5, 100, -7};
  const PendingOp pending[] = {
    OP_NONE, OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_NONE, OP_ADD, OP_NONE
  };
  s->x = xs[case_index];
  s->y = ys[case_index];
  s->pending = pending[case_index];
  s->entering = false;
  s->decimal_entry = false;
  s->exponent_entry = false;
  s->pending_memory = -1;
  for (int i = 0; i < MEMORY_SIZE; i++) {
    s->memory[i] = (double)((case_index + 1) * 10 + i);
  }
  s->memory[0] = case_index == 0 ? 0.0 : (double)(case_index + 1);
  s->memory[1] = case_index == 1 ? 0.0 : (double)(case_index + 2);
}

static void make_signature(const Sequence *seq, char *out, size_t out_size) {
  out[0] = '\0';
  for (int i = 0; i < 8; i++) {
    SantronState s;
    setup_case(&s, i);
    load_sequence(&s, seq);
    run_program(&s, 100);
    append_state_signature(out, out_size, &s);
    strncat(out, "|", out_size - strlen(out) - 1);
  }
}

static int full_operation_set(Operation *ops, int capacity) {
  const int single_cell[] = {
    KEY_ADD, KEY_SUB, KEY_MUL, KEY_DIV, KEY_YX, KEY_INV, KEY_SQR, KEY_SQRT,
    KEY_SIN, KEY_COS, KEY_TAN, KEY_ASIN, KEY_ACOS, KEY_ATAN,
    KEY_LN, KEY_LOG, KEY_EX, KEY_TENX,
    KEY_CLEAR, KEY_SIGN, KEY_EXP, KEY_SWAP, KEY_PI, KEY_DOT, KEY_PS,
    KEY_EQ, KEY_LPAREN, KEY_RPAREN
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
  return count;
}

static Sequence random_sequence(const Operation *ops, int op_count, int max_cells) {
  Sequence seq = {0};
  int target = 1 + rng_int(max_cells);
  int guard = 0;
  while (seq.len < target && guard++ < 100) {
    Operation op = ops[rng_int(op_count)];
    if (seq.len + op.len > target) continue;
    for (int i = 0; i < op.len; i++) seq.codes[seq.len++] = op.codes[i];
  }
  if (seq.len == 0) {
    seq.codes[seq.len++] = KEY_EQ;
  }
  return seq;
}

static void print_code_array(FILE *out, const Sequence *seq) {
  fprintf(out, "[");
  for (int i = 0; i < seq->len; i++) {
    fprintf(out, "%s%d", i ? "," : "", seq->codes[i]);
  }
  fprintf(out, "]");
}

static void print_name_array(FILE *out, const Sequence *seq) {
  fprintf(out, "[");
  for (int i = 0; i < seq->len; i++) {
    fprintf(out, "%s\"%s\"", i ? "," : "", code_name(seq->codes[i]));
  }
  fprintf(out, "]");
}

static void emit_rule(FILE *out, const Sequence *lhs, const Sequence *rhs,
                      const char *source, int tested) {
  if (lhs->len <= rhs->len) return;
  fprintf(out, "{\"lhs\":");
  print_code_array(out, lhs);
  fprintf(out, ",\"rhs\":");
  print_code_array(out, rhs);
  fprintf(out, ",\"lhs_names\":");
  print_name_array(out, lhs);
  fprintf(out, ",\"rhs_names\":");
  print_name_array(out, rhs);
  fprintf(out, ",\"cells_saved\":%d,\"tested\":%d,\"source\":\"%s\"}\n",
          lhs->len - rhs->len, tested, source);
}

static void emit_manual_rules(FILE *out) {
  for (int digit = 0; digit <= 9; digit++) {
    Sequence lhs = {{KEY_RCL, (uint16_t)(KEY_DIGIT0 + digit), KEY_RCL, (uint16_t)(KEY_DIGIT0 + digit)}, 4};
    Sequence rhs = {{KEY_RCL, (uint16_t)(KEY_DIGIT0 + digit)}, 2};
    emit_rule(out, &lhs, &rhs, "manual", 8);

    lhs = (Sequence){{KEY_RCL, (uint16_t)(KEY_DIGIT0 + digit), KEY_STO, (uint16_t)(KEY_DIGIT0 + digit)}, 4};
    rhs = (Sequence){{KEY_RCL, (uint16_t)(KEY_DIGIT0 + digit)}, 2};
    emit_rule(out, &lhs, &rhs, "manual", 8);

    lhs = (Sequence){{KEY_STO, (uint16_t)(KEY_DIGIT0 + digit), KEY_RCL, (uint16_t)(KEY_DIGIT0 + digit)}, 4};
    rhs = (Sequence){{KEY_STO, (uint16_t)(KEY_DIGIT0 + digit)}, 2};
    emit_rule(out, &lhs, &rhs, "manual", 8);
  }

  Sequence lhs = {{KEY_EQ, KEY_EQ}, 2};
  Sequence rhs = {{KEY_EQ}, 1};
  emit_rule(out, &lhs, &rhs, "manual", 8);

  lhs = (Sequence){{KEY_CLEAR, KEY_CLEAR}, 2};
  rhs = (Sequence){{KEY_CLEAR}, 1};
  emit_rule(out, &lhs, &rhs, "manual", 8);

}

static Entry *find_entry(const char *signature) {
  uint64_t h = hash_string(signature);
  for (int probe = 0; probe < HASH_SIZE; probe++) {
    Entry *entry = &table[(h + (uint64_t)probe) & (HASH_SIZE - 1)];
    if (!entry->signature) return entry;
    if (strcmp(entry->signature, signature) == 0) return entry;
  }
  fprintf(stderr, "hash table full\n");
  exit(1);
}

static int parse_int_arg(int argc, char **argv, const char *name, int default_value) {
  for (int i = 1; i + 1 < argc; i++) {
    if (strcmp(argv[i], name) == 0) return atoi(argv[i + 1]);
  }
  return default_value;
}

static const char *parse_string_arg(int argc, char **argv, const char *name, const char *default_value) {
  for (int i = 1; i + 1 < argc; i++) {
    if (strcmp(argv[i], name) == 0) return argv[i + 1];
  }
  return default_value;
}

int main(int argc, char **argv) {
  int samples = parse_int_arg(argc, argv, "--samples", DEFAULT_SAMPLES);
  int max_cells = parse_int_arg(argc, argv, "--max-cells", MAX_CELLS);
  int seed = parse_int_arg(argc, argv, "--seed", (int)time(NULL));
  const char *out_path = parse_string_arg(argc, argv, "--out", "tools/peephole-rules.jsonl");

  if (max_cells < 1 || max_cells > MAX_CELLS) {
    fprintf(stderr, "max cells must be 1..%d\n", MAX_CELLS);
    return 2;
  }

  rng_state = (uint64_t)(unsigned int)seed;
  if (!rng_state) rng_state = 1;

  FILE *out = fopen(out_path, "w");
  if (!out) {
    perror(out_path);
    return 1;
  }

  Operation ops[MAX_OPS];
  int op_count = full_operation_set(ops, MAX_OPS);
  fprintf(stderr, "seed=%d samples=%d max_cells=%d ops=%d out=%s\n",
          seed, samples, max_cells, op_count, out_path);

  emit_manual_rules(out);

  int emitted = 32;
  for (int i = 0; i < samples; i++) {
    Sequence seq = random_sequence(ops, op_count, max_cells);
    char signature[SIGNATURE_SIZE];
    make_signature(&seq, signature, sizeof(signature));
    Entry *entry = find_entry(signature);
    if (!entry->signature) {
      entry->signature = copy_string(signature);
      entry->best = seq;
    } else if (seq.len < entry->best.len) {
      emit_rule(out, &entry->best, &seq, "mined", 8);
      entry->best = seq;
      emitted++;
    } else if (seq.len > entry->best.len) {
      emit_rule(out, &seq, &entry->best, "mined", 8);
      emitted++;
    }
  }

  fclose(out);
  fprintf(stderr, "emitted roughly %d rules\n", emitted);
  return 0;
}
