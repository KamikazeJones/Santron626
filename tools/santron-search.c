#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SANTRON_VM_NO_MAIN
#include "santron-vm.c"

#define MAX_CELLS PROGRAM_SIZE
#define MAX_OPS 256
#define MAX_PROGRAM_OPS PROGRAM_SIZE
#define MAX_CASES 256
#define MAX_KEYS 256
#define MAX_LINE 4096

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

typedef struct {
  bool has_x;
  bool has_y;
  bool has_pc;
  bool has_memory[MEMORY_SIZE];
  bool has_pause;
  bool has_running;
  bool has_pending_none;
  bool has_pending_memory_none;
  bool has_paren;
  bool has_exponent;
  bool has_loop;
  double x;
  double y;
  double memory[MEMORY_SIZE];
  int pc;
  int pause;
  int running;
  int paren;
  int exponent;
  int loop; /* -1: done (x < 0), 1: continue (x >= 0) */
} StateSpec;

typedef struct {
  StateSpec init;
  StateSpec expect;
  uint16_t keys[MAX_KEYS];
  int key_count;
  int line_no;
} TargetCase;

typedef struct {
  char name[128];
  int max_cells;
  uint16_t prefix[MAX_CELLS];
  int prefix_len;
  uint16_t append[MAX_CELLS];
  int append_len;
  uint16_t seed[MAX_CELLS];
  int seed_len;
  TargetCase cases[MAX_CASES];
  int case_count;
} Target;

static uint64_t rng_state = 0x123456789abcdef0ULL;
static bool verbose_failures = false;

typedef struct {
  bool enabled;
  int best_cells;
  int tested;
  int passed;
  FILE *log_file;
  bool has_promoted;
  Program promoted_program;
} PromotionState;

static void close_promotion_log(PromotionState *promotion) {
  if (!promotion->log_file) return;
  fflush(promotion->log_file);
  fclose(promotion->log_file);
  promotion->log_file = NULL;
}

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

static bool is_memory_code(int code) {
  return code == KEY_STO || code == KEY_RCL || code == KEY_MPLUS ||
         code == KEY_MMINUS || code == KEY_MULMEM || code == KEY_DIVMEM;
}

static bool parse_key_token(const char *token, int *code) {
  if (strlen(token) == 1 && token[0] >= '0' && token[0] <= '9') {
    *code = KEY_DIGIT0 + token[0] - '0';
    return true;
  }
  if (strcmp(token, "+") == 0) *code = KEY_ADD;
  else if (strcmp(token, "-") == 0) *code = KEY_SUB;
  else if (strcmp(token, "*") == 0) *code = KEY_MUL;
  else if (strcmp(token, "/") == 0) *code = KEY_DIV;
  else if (strcmp(token, "Y^X") == 0) *code = KEY_YX;
  else if (strcmp(token, "1/X") == 0) *code = KEY_INV;
  else if (strcmp(token, "X^2") == 0) *code = KEY_SQR;
  else if (strcmp(token, "SQRT") == 0) *code = KEY_SQRT;
  else if (strcmp(token, "SIN") == 0) *code = KEY_SIN;
  else if (strcmp(token, "COS") == 0) *code = KEY_COS;
  else if (strcmp(token, "TAN") == 0) *code = KEY_TAN;
  else if (strcmp(token, "ASIN") == 0) *code = KEY_ASIN;
  else if (strcmp(token, "ACOS") == 0) *code = KEY_ACOS;
  else if (strcmp(token, "ATAN") == 0) *code = KEY_ATAN;
  else if (strcmp(token, "LN") == 0) *code = KEY_LN;
  else if (strcmp(token, "LOG") == 0) *code = KEY_LOG;
  else if (strcmp(token, "E^X") == 0) *code = KEY_EX;
  else if (strcmp(token, "10^X") == 0) *code = KEY_TENX;
  else if (strcmp(token, "M+") == 0) *code = KEY_MPLUS;
  else if (strcmp(token, "M-") == 0) *code = KEY_MMINUS;
  else if (strcmp(token, "M*") == 0) *code = KEY_MULMEM;
  else if (strcmp(token, "M/") == 0) *code = KEY_DIVMEM;
  else if (strcmp(token, "STO") == 0) *code = KEY_STO;
  else if (strcmp(token, "RCL") == 0) *code = KEY_RCL;
  else if (strcmp(token, "C/CE") == 0) *code = KEY_CLEAR;
  else if (strcmp(token, "+/-") == 0) *code = KEY_SIGN;
  else if (strcmp(token, "EXP") == 0) *code = KEY_EXP;
  else if (strcmp(token, "X<>Y") == 0) *code = KEY_SWAP;
  else if (strcmp(token, "PI") == 0) *code = KEY_PI;
  else if (strcmp(token, ".") == 0) *code = KEY_DOT;
  else if (strcmp(token, "PS") == 0) *code = KEY_PS;
  else if (strcmp(token, "=") == 0) *code = KEY_EQ;
  else if (strcmp(token, "(") == 0) *code = KEY_LPAREN;
  else if (strcmp(token, ")") == 0) *code = KEY_RPAREN;
  else if (strcmp(token, "R/S") == 0) *code = KEY_RS;
  else if (strcmp(token, "GOTO") == 0) *code = KEY_GOTO;
  else if (strcmp(token, "SKP") == 0) *code = KEY_SKP;
  else return false;
  return true;
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

static void strip_comment(char *line) {
  char *hash = strchr(line, '#');
  if (hash) *hash = '\0';
}

static char *trim(char *line) {
  while (isspace((unsigned char)*line)) line++;
  char *end = line + strlen(line);
  while (end > line && isspace((unsigned char)end[-1])) end--;
  *end = '\0';
  return line;
}

static void die_parse(const char *path, int line_no, const char *message) {
  fprintf(stderr, "%s:%d: %s\n", path, line_no, message);
  exit(2);
}

static int parse_key_sequence(char *text, uint16_t *codes, int max_codes, const char *path, int line_no) {
  int count = 0;
  for (char *tok = strtok(text, " \t\r\n"); tok; tok = strtok(NULL, " \t\r\n")) {
    int code = 0;
    if (!parse_key_token(tok, &code)) die_parse(path, line_no, "unknown key token");
    if (count >= max_codes) die_parse(path, line_no, "too many key tokens");
    codes[count++] = (uint16_t)code;
  }
  return count;
}

static bool parse_assignment(const char *token, char *name, size_t name_size, double *value) {
  const char *eq = strchr(token, '=');
  if (!eq || eq == token || eq[1] == '\0') return false;
  size_t len = (size_t)(eq - token);
  if (len >= name_size) return false;
  memcpy(name, token, len);
  name[len] = '\0';
  char *end = NULL;
  *value = strtod(eq + 1, &end);
  return end != eq + 1 && *end == '\0';
}

static void apply_numeric_assignment(StateSpec *spec, const char *name, double value, const char *path, int line_no) {
  if (name[0] == 'M' && isdigit((unsigned char)name[1]) && name[2] == '\0') {
    int idx = name[1] - '0';
    spec->has_memory[idx] = true;
    spec->memory[idx] = value;
  } else if (strcmp(name, "X") == 0) {
    spec->has_x = true;
    spec->x = value;
  } else if (strcmp(name, "Y") == 0) {
    spec->has_y = true;
    spec->y = value;
  } else if (strcmp(name, "pc") == 0 || strcmp(name, "PC") == 0) {
    spec->has_pc = true;
    spec->pc = (int)value;
  } else if (strcmp(name, "pause") == 0 || strcmp(name, "paused") == 0) {
    spec->has_pause = true;
    spec->pause = (int)value;
  } else if (strcmp(name, "running") == 0) {
    spec->has_running = true;
    spec->running = (int)value;
  } else if (strcmp(name, "paren") == 0) {
    spec->has_paren = true;
    spec->paren = (int)value;
  } else if (strcmp(name, "exponent") == 0) {
    spec->has_exponent = true;
    spec->exponent = (int)value;
  } else {
    die_parse(path, line_no, "unknown numeric state field");
  }
}

static void parse_state_spec(char *text, StateSpec *spec, bool expect, const char *path, int line_no) {
  for (char *tok = strtok(text, " \t\r\n"); tok; tok = strtok(NULL, " \t\r\n")) {
    if (strcmp(tok, "pending=none") == 0) {
      spec->has_pending_none = true;
    } else if (strcmp(tok, "pending-memory=none") == 0) {
      spec->has_pending_memory_none = true;
    } else if (strcmp(tok, "loop=continue") == 0) {
      spec->has_loop = true;
      spec->loop = 1;
    } else if (strcmp(tok, "loop=done") == 0) {
      spec->has_loop = true;
      spec->loop = -1;
    } else {
      char name[64];
      double value = 0.0;
      if (!parse_assignment(tok, name, sizeof(name), &value)) {
        die_parse(path, line_no, expect ? "invalid expect assignment" : "invalid init assignment");
      }
      apply_numeric_assignment(spec, name, value, path, line_no);
    }
  }
}

static TargetCase *current_case(Target *target, const char *path, int line_no) {
  if (target->case_count == 0) die_parse(path, line_no, "line requires a preceding case");
  return &target->cases[target->case_count - 1];
}

static void parse_target(const char *path, Target *target) {
  memset(target, 0, sizeof(*target));
  target->max_cells = 72;
  FILE *f = fopen(path, "r");
  if (!f) {
    perror(path);
    exit(2);
  }

  char line[MAX_LINE];
  int line_no = 0;
  while (fgets(line, sizeof(line), f)) {
    line_no++;
    strip_comment(line);
    char *p = trim(line);
    if (*p == '\0') continue;

    char *space = p;
    while (*space && !isspace((unsigned char)*space)) space++;
    char keyword[64];
    size_t key_len = (size_t)(space - p);
    if (key_len >= sizeof(keyword)) die_parse(path, line_no, "keyword too long");
    memcpy(keyword, p, key_len);
    keyword[key_len] = '\0';
    char *rest = trim(space);

    if (strcmp(keyword, "name") == 0) {
      snprintf(target->name, sizeof(target->name), "%s", rest);
    } else if (strcmp(keyword, "max-cells") == 0) {
      target->max_cells = atoi(rest);
      if (target->max_cells <= 0 || target->max_cells > PROGRAM_SIZE) {
        die_parse(path, line_no, "max-cells must be 1..100");
      }
    } else if (strcmp(keyword, "prefix") == 0) {
      target->prefix_len = parse_key_sequence(rest, target->prefix, MAX_CELLS, path, line_no);
    } else if (strcmp(keyword, "append") == 0) {
      target->append_len = parse_key_sequence(rest, target->append, MAX_CELLS, path, line_no);
    } else if (strcmp(keyword, "seed") == 0) {
      target->seed_len = parse_key_sequence(rest, target->seed, MAX_CELLS, path, line_no);
    } else if (strcmp(keyword, "case") == 0) {
      if (target->case_count >= MAX_CASES) die_parse(path, line_no, "too many cases");
      TargetCase *tc = &target->cases[target->case_count++];
      memset(tc, 0, sizeof(*tc));
      tc->line_no = line_no;
    } else if (strcmp(keyword, "init") == 0) {
      parse_state_spec(rest, &current_case(target, path, line_no)->init, false, path, line_no);
    } else if (strcmp(keyword, "expect") == 0) {
      parse_state_spec(rest, &current_case(target, path, line_no)->expect, true, path, line_no);
    } else if (strcmp(keyword, "keys") == 0) {
      TargetCase *tc = current_case(target, path, line_no);
      tc->key_count = parse_key_sequence(rest, tc->keys, MAX_KEYS, path, line_no);
    } else {
      die_parse(path, line_no, "unknown directive");
    }
  }
  fclose(f);

  if (target->case_count == 0) {
    fprintf(stderr, "%s: no cases defined\n", path);
    exit(2);
  }
  if (target->name[0] == '\0') snprintf(target->name, sizeof(target->name), "unnamed");
}

static void seed_from_codes(Program *p, const uint16_t *codes, int len) {
  memset(p, 0, sizeof(*p));
  int i = 0;
  while (i < len && p->op_count < MAX_PROGRAM_OPS) {
    Operation *op = &p->ops[p->op_count++];
    op->codes[0] = codes[i++];
    op->len = 1;
    if (is_memory_code(op->codes[0]) && i < len && digit_from_code(codes[i]) >= 0) {
      op->codes[1] = codes[i++];
      op->len = 2;
    } else if (op->codes[0] == KEY_GOTO && i + 1 < len &&
               digit_from_code(codes[i]) >= 0 && digit_from_code(codes[i + 1]) >= 0) {
      op->codes[1] = codes[i++];
      op->codes[2] = codes[i++];
      op->len = 3;
    }
  }
}

static void load_candidate(SantronState *s, const uint16_t *codes, int len, const Target *target) {
  for (int i = 0; i < PROGRAM_SIZE; i++) s->program[i] = KEY_BLANK;
  int pc = 0;
  for (int i = 0; i < target->prefix_len && pc < PROGRAM_SIZE; i++) s->program[pc++] = (int)target->prefix[i];
  for (int i = 0; i < len && pc < PROGRAM_SIZE; i++) s->program[pc++] = (int)codes[i];
  for (int i = 0; i < target->append_len && pc < PROGRAM_SIZE; i++) s->program[pc++] = (int)target->append[i];
  if (target->append_len == 0 && pc < PROGRAM_SIZE) s->program[pc++] = KEY_RS;
  s->pc = 0;
}

static void apply_init(SantronState *s, const StateSpec *spec) {
  if (spec->has_x) set_x(s, spec->x);
  if (spec->has_y) s->y = spec->y;
  if (spec->has_pc) s->pc = spec->pc;
  if (spec->has_pause) s->paused = spec->pause != 0;
  if (spec->has_running) s->running = spec->running != 0;
  for (int i = 0; i < MEMORY_SIZE; i++) {
    if (spec->has_memory[i]) s->memory[i] = spec->memory[i];
  }
}

static void fill_unspecified_state(SantronState *s, const StateSpec *spec, int variant) {
  static const double values[][MEMORY_SIZE] = {
    {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {7.0, -3.0, 99.0, 1.0, 42.0, 5.0, 8.0, -1.0, 13.0, 2.0},
    {-9.0, 11.0, 17.0, 6.0, -2.0, 31.0, -4.0, 23.0, 3.0, -7.0},
  };
  int index = variant % (int)(sizeof(values) / sizeof(values[0]));
  for (int i = 0; i < MEMORY_SIZE; i++) {
    if (!spec->has_memory[i]) s->memory[i] = values[index][i];
  }
  if (!spec->has_x) set_x(s, values[index][0]);
  if (!spec->has_y) s->y = values[index][1];
}

static void run_keys(SantronState *s, const TargetCase *tc) {
  for (int i = 0; i < tc->key_count; i++) {
    int code = (int)tc->keys[i];
    execute_key(s, code, false);
    if (code == KEY_RS) run_program(s, 1000);
  }
}

static bool check_numeric(double actual, double expected, double *error) {
  if (nearly_equal(actual, expected)) return true;
  if (isfinite(actual) && isfinite(expected)) *error += fabs(actual - expected);
  else *error += 1e6;
  return false;
}

static bool check_case(const SantronState *s, const StateSpec *expect, double *error, double *penalty) {
  bool ok = true;
  if (expect->has_x) ok = check_numeric(s->x, expect->x, error) && ok;
  if (expect->has_y) ok = check_numeric(s->y, expect->y, error) && ok;
  if (expect->has_pc) ok = check_numeric((double)s->pc, (double)expect->pc, error) && ok;
  for (int i = 0; i < MEMORY_SIZE; i++) {
    if (expect->has_memory[i]) ok = check_numeric(s->memory[i], expect->memory[i], error) && ok;
  }
  if (expect->has_pause && ((s->paused ? 1 : 0) != expect->pause)) {
    *penalty += 1000.0;
    ok = false;
  }
  if (expect->has_running && ((s->running ? 1 : 0) != expect->running)) {
    *penalty += 1000.0;
    ok = false;
  }
  if (expect->has_pending_none && s->pending != OP_NONE) {
    *penalty += 1000.0;
    ok = false;
  }
  if (expect->has_pending_memory_none && s->pending_memory >= 0) {
    *penalty += 100.0;
    ok = false;
  }
  if (expect->has_paren && s->paren_depth != expect->paren) {
    *penalty += 1000.0;
    ok = false;
  }
  if (expect->has_exponent && ((s->exponent_entry ? 1 : 0) != expect->exponent)) {
    *penalty += 100.0;
    ok = false;
  }
  if (expect->has_loop) {
    bool loop_ok = expect->loop > 0 ? s->x >= 0.0 : s->x < 0.0;
    if (!loop_ok) {
      *penalty += 1000.0;
      ok = false;
    }
  }
  return ok;
}

static Score score_program(const Program *p, const Target *target) {
  uint16_t codes[MAX_CELLS];
  int len = flatten_program(p, codes, target->max_cells);
  Score score = {0};
  score.cells = len;
  const int variants = 3;
  score.total = target->case_count * variants;

  for (int i = 0; i < target->case_count; i++) {
    for (int variant = 0; variant < variants; variant++) {
      SantronState s;
      init_state(&s);
      fill_unspecified_state(&s, &target->cases[i].init, variant);
      load_candidate(&s, codes, len, target);
      apply_init(&s, &target->cases[i].init);
      run_keys(&s, &target->cases[i]);
      double error = 0.0;
      double penalty = 0.0;
      bool ok = check_case(&s, &target->cases[i].expect, &error, &penalty);
      if (ok) {
        score.exact++;
      } else if (verbose_failures) {
        fprintf(stderr,
                "case starting at line %d variant %d failed: x=%.12g y=%.12g pc=%d paused=%d running=%d pending=%d pending_memory=%d paren=%d exponent=%d\n",
                target->cases[i].line_no, variant, s.x, s.y, s.pc,
                s.paused ? 1 : 0, s.running ? 1 : 0, (int)s.pending,
                s.pending_memory, s.paren_depth, s.exponent_entry ? 1 : 0);
        for (int m = 0; m < MEMORY_SIZE; m++) {
          fprintf(stderr, " M%d=%.12g", m, s.memory[m]);
        }
        fprintf(stderr, "\n");
      }
      score.error += error;
      score.penalty += penalty;
    }
  }

  score.cost =
    (double)(score.total - score.exact) * 100000.0 +
    score.error * 100.0 +
    score.penalty +
    (double)score.cells;
  return score;
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
  for (int target = 0; target < PROGRAM_SIZE; target++) {
    if (count >= capacity) return count;
    ops[count].codes[0] = KEY_GOTO;
    ops[count].codes[1] = (uint16_t)(KEY_DIGIT0 + target / 10);
    ops[count].codes[2] = (uint16_t)(KEY_DIGIT0 + target % 10);
    ops[count].len = 3;
    count++;
  }
  return count;
}

static void mutate_program(Program *dst, const Program *src, const Operation *ops, int op_count, int max_cells) {
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
    } else if (dst->ops[idx].len == 3 && dst->ops[idx].codes[0] == KEY_GOTO) {
      int target = rng_int(PROGRAM_SIZE);
      dst->ops[idx].codes[1] = (uint16_t)(KEY_DIGIT0 + target / 10);
      dst->ops[idx].codes[2] = (uint16_t)(KEY_DIGIT0 + target % 10);
    } else {
      dst->ops[idx] = ops[rng_int(op_count)];
    }
  }

  while (program_cell_count(dst) > max_cells && dst->op_count > 0) {
    int idx = rng_int(dst->op_count);
    for (int i = idx; i + 1 < dst->op_count; i++) dst->ops[i] = dst->ops[i + 1];
    dst->op_count--;
  }
}

static void random_program(Program *p, const Operation *ops, int op_count,
                           int min_cells, int max_cells) {
  memset(p, 0, sizeof(*p));
  if (min_cells < 0) min_cells = 0;
  if (max_cells < min_cells) max_cells = min_cells;
  if (max_cells > MAX_CELLS) max_cells = MAX_CELLS;

  int target_cells = min_cells + rng_int(max_cells - min_cells + 1);
  while (program_cell_count(p) < target_cells && p->op_count < MAX_PROGRAM_OPS) {
    Operation op = ops[rng_int(op_count)];
    if (program_cell_count(p) + op.len > max_cells) continue;
    p->ops[p->op_count++] = op;
  }
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

static void fprint_program_keys(FILE *f, const Program *p) {
  uint16_t codes[MAX_CELLS];
  int len = flatten_program(p, codes, MAX_CELLS);
  for (int i = 0; i < len; i++) fprintf(f, "%s%s", i ? " " : "", code_name(codes[i]));
  fprintf(f, "\n");
}

static void fprint_program_listing(FILE *f, const Program *p) {
  uint16_t codes[MAX_CELLS];
  int len = flatten_program(p, codes, MAX_CELLS);
  for (int i = 0; i < len; i++) fprintf(f, "%02d %03d %s\n", i, codes[i], code_name(codes[i]));
}

static bool has_duplicate_digits(const int digits[4]) {
  for (int i = 0; i < 4; i++) {
    for (int j = i + 1; j < 4; j++) {
      if (digits[i] == digits[j]) return true;
    }
  }
  return false;
}

static int encode_mastermind_code(const int code[4]) {
  static const int pow10[] = {
    1, 10, 100, 1000, 10000, 100000, 1000000
  };
  int encoded = 0;
  for (int pos = 0; pos < 4; pos++) {
    encoded += (pos + 1) * pow10[code[pos]];
  }
  return encoded;
}

static void expected_mastermind_score(const int code[4], const int guess[4], int *black, int *white) {
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

static bool close_to_int(double actual, int expected) {
  return fabs(actual - (double)expected) <= 1e-8;
}

static void press_memory_key(SantronState *s, int command, int digit) {
  execute_key(s, command, false);
  execute_key(s, KEY_DIGIT0 + digit, false);
}

static int append_code(uint16_t *program, int pc, int max_pc, int code) {
  if (pc < max_pc) program[pc] = (uint16_t)code;
  return pc + 1;
}

static int append_memory_command(uint16_t *program, int pc, int max_pc, int command, int idx) {
  pc = append_code(program, pc, max_pc, command);
  pc = append_code(program, pc, max_pc, KEY_DIGIT0 + idx);
  return pc;
}

static int append_goto(uint16_t *program, int pc, int max_pc, int addr) {
  pc = append_code(program, pc, max_pc, KEY_GOTO);
  pc = append_code(program, pc, max_pc, KEY_DIGIT0 + addr / 10);
  pc = append_code(program, pc, max_pc, KEY_DIGIT0 + addr % 10);
  return pc;
}

static int build_mastermind_program(const Program *extract, uint16_t *program, int max_program) {
  uint16_t extract_codes[MAX_CELLS];
  int extract_len = flatten_program(extract, extract_codes, MAX_CELLS);
  int pc = 0;

  for (int i = 0; i < max_program; i++) program[i] = KEY_BLANK;

  pc = append_code(program, pc, max_program, KEY_DIGIT0 + 1);
  pc = append_memory_command(program, pc, max_program, KEY_STO, 0);
  pc = append_memory_command(program, pc, max_program, KEY_STO, 2);
  pc = append_code(program, pc, max_program, KEY_RS);
  pc = append_memory_command(program, pc, max_program, KEY_STO, 4);

  for (int i = 0; i < extract_len && pc < max_program; i++) {
    program[pc++] = extract_codes[i];
  }

  pc = append_memory_command(program, pc, max_program, KEY_RCL, 7);
  pc = append_code(program, pc, max_program, KEY_SUB);
  pc = append_code(program, pc, max_program, KEY_SIGN);
  pc = append_code(program, pc, max_program, KEY_SKP);
  pc = append_memory_command(program, pc, max_program, KEY_RCL, 0);
  pc = append_memory_command(program, pc, max_program, KEY_MMINUS, 9);

  pc = append_memory_command(program, pc, max_program, KEY_RCL, 2);
  pc = append_code(program, pc, max_program, KEY_MUL);
  pc = append_code(program, pc, max_program, KEY_SIGN);
  pc = append_code(program, pc, max_program, KEY_SUB);
  pc = append_code(program, pc, max_program, KEY_DIGIT0 + 4);
  pc = append_code(program, pc, max_program, KEY_SWAP);
  pc = append_code(program, pc, max_program, KEY_SKP);
  pc = append_memory_command(program, pc, max_program, KEY_RCL, 0);
  pc = append_memory_command(program, pc, max_program, KEY_MMINUS, 9);
  pc = append_memory_command(program, pc, max_program, KEY_MPLUS, 1);

  pc = append_code(program, pc, max_program, KEY_DIGIT0 + 1);
  pc = append_memory_command(program, pc, max_program, KEY_MPLUS, 2);

  pc = append_memory_command(program, pc, max_program, KEY_RCL, 2);
  pc = append_code(program, pc, max_program, KEY_EQ);
  pc = append_code(program, pc, max_program, KEY_SKP);
  pc = append_goto(program, pc, max_program, 5);

  pc = append_code(program, pc, max_program, KEY_RS);
  pc = append_goto(program, pc, max_program, 0);
  return pc <= max_program ? pc : max_program + 1;
}

static bool run_mastermind_case(const SantronState *base, int input_pause_pc, int result_pause_pc,
                                const int code[4], const int guess[4]) {
  int expected_black = 0;
  int expected_white = 0;
  expected_mastermind_score(code, guess, &expected_black, &expected_white);

  SantronState s = *base;
  s.memory[1] = 0.0;
  s.memory[8] = (double)encode_mastermind_code(code);
  s.memory[9] = 4.0;

  run_program(&s, 1000);
  for (int i = 0; i < 4; i++) {
    execute_key(&s, KEY_DIGIT0 + guess[i], false);
    run_program(&s, 1000);
  }

  bool ok = true;
  ok = ok && s.paused && s.pc == result_pause_pc;
  ok = ok && close_to_int(s.memory[0], 1);
  ok = ok && close_to_int(s.memory[1], expected_black);
  ok = ok && close_to_int(s.memory[2], 5);
  ok = ok && close_to_int(s.memory[9], expected_white);

  press_memory_key(&s, KEY_RCL, 1);
  ok = ok && close_to_int(s.x, expected_black);
  press_memory_key(&s, KEY_RCL, 9);
  ok = ok && close_to_int(s.x, expected_white);
  execute_key(&s, KEY_DIGIT0 + 0, false);
  press_memory_key(&s, KEY_STO, 1);
  execute_key(&s, KEY_DIGIT0 + 4, false);
  press_memory_key(&s, KEY_STO, 9);

  run_program(&s, 1000);
  ok = ok && s.paused && s.pc == input_pause_pc;
  ok = ok && close_to_int(s.memory[0], 1);
  ok = ok && close_to_int(s.memory[1], 0);
  ok = ok && close_to_int(s.memory[2], 1);
  ok = ok && close_to_int(s.memory[9], 4);
  return ok;
}

static bool mastermind_full_test_candidate(const Program *extract, int *tested_out, int *program_cells_out) {
  uint16_t program_codes[PROGRAM_SIZE];
  int used_cells = build_mastermind_program(extract, program_codes, PROGRAM_SIZE);
  if (used_cells > PROGRAM_SIZE) return false;
  if (program_cells_out) *program_cells_out = used_cells;

  SantronState base;
  init_state(&base);
  for (int i = 0; i < PROGRAM_SIZE; i++) base.program[i] = (int)program_codes[i];

  const int input_pause_pc = 6;
  int result_pause_pc = -1;
  for (int i = 6; i < PROGRAM_SIZE; i++) {
    if (base.program[i] == KEY_RS) result_pause_pc = i + 1;
  }
  if (result_pause_pc < 0) return false;

  int tested = 0;
  for (int c0 = 1; c0 <= 6; c0++) {
    for (int c1 = 1; c1 <= 6; c1++) {
      for (int c2 = 1; c2 <= 6; c2++) {
        for (int c3 = 1; c3 <= 6; c3++) {
          int code[4] = {c0, c1, c2, c3};
          if (has_duplicate_digits(code)) continue;

          for (int g0 = 1; g0 <= 6; g0++) {
            for (int g1 = 1; g1 <= 6; g1++) {
              for (int g2 = 1; g2 <= 6; g2++) {
                for (int g3 = 1; g3 <= 6; g3++) {
                  int guess[4] = {g0, g1, g2, g3};
                  if (has_duplicate_digits(guess)) continue;
                  if (!run_mastermind_case(&base, input_pause_pc, result_pause_pc, code, guess)) {
                    if (tested_out) *tested_out = tested;
                    return false;
                  }
                  tested++;
                }
              }
            }
          }
        }
      }
    }
  }

  if (tested_out) *tested_out = tested;
  return true;
}

static bool try_promote_mastermind(const Program *candidate, const Score *score,
                                   PromotionState *promotion, const char *where) {
  if (!promotion || !promotion->enabled) return false;
  if (score->exact != score->total || score->penalty != 0.0) return false;
  if (score->cells >= promotion->best_cells) return false;

  promotion->tested++;
  int full_tested = 0;
  int program_cells = 0;
  bool ok = mastermind_full_test_candidate(candidate, &full_tested, &program_cells);
  printf("\npromotion check at %s: extract cells=%d mastermind cells=%d full-tested=%d result=%s\n",
         where, score->cells, program_cells, full_tested, ok ? "PASS" : "FAIL");
  print_program(candidate, score);
  fflush(stdout);

  if (!ok) return false;

  promotion->passed++;
  promotion->best_cells = score->cells;
  promotion->promoted_program = *candidate;
  promotion->has_promoted = true;
  printf("promotion accepted: new seed has %d extract cells\n", score->cells);
  if (promotion->log_file) {
    fprintf(promotion->log_file,
            "promotion where=\"%s\" extract_cells=%d mastermind_cells=%d full_tested=%d exact=%d/%d error=%.12g penalty=%.12g cost=%.12g\n",
            where, score->cells, program_cells, full_tested, score->exact,
            score->total, score->error, score->penalty, score->cost);
    fprintf(promotion->log_file, "keys: ");
    fprint_program_keys(promotion->log_file, candidate);
    fprintf(promotion->log_file, "listing:\n");
    fprint_program_listing(promotion->log_file, candidate);
    fprintf(promotion->log_file, "---\n");
    fflush(promotion->log_file);
  }
  fflush(stdout);
  return true;
}

static int parse_int_arg(int argc, char **argv, const char *name, int default_value) {
  for (int i = 1; i + 1 < argc; i++) {
    if (strcmp(argv[i], name) == 0) return atoi(argv[i + 1]);
  }
  return default_value;
}

static uint64_t parse_u64_value(const char *text) {
  char *end = NULL;
  unsigned long long value = strtoull(text, &end, 10);
  if (end == text || *end != '\0') {
    fprintf(stderr, "invalid unsigned integer: %s\n", text);
    exit(2);
  }
  return (uint64_t)value;
}

static uint64_t current_unix_millis(void) {
#if defined(TIME_UTC)
  struct timespec ts;
  if (timespec_get(&ts, TIME_UTC) == TIME_UTC) {
    return (uint64_t)ts.tv_sec * 1000ULL + (uint64_t)(ts.tv_nsec / 1000000L);
  }
#endif
  return (uint64_t)time(NULL) * 1000ULL;
}

static const char *parse_string_arg(int argc, char **argv, const char *name, const char *default_value) {
  for (int i = 1; i + 1 < argc; i++) {
    if (strcmp(argv[i], name) == 0) return argv[i + 1];
  }
  return default_value;
}

static bool has_flag(int argc, char **argv, const char *name) {
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], name) == 0) return true;
  }
  return false;
}

static void usage(const char *argv0) {
  fprintf(stderr,
          "usage: %s --target file [--iterations n] [--restarts n] [--seed n|--seed-ms] [--candidate \"keys...\"] [--random-start] [--random-min-cells n] [--random-max-cells n] [--promote-mastermind] [--promotion-log file] [--check]\n",
          argv0);
}

static void run_restart(const Target *target, const Operation *ops, int op_count,
                        const Program *seed_program, int restart_index,
                        int restart_count, uint64_t seed, int iterations,
                        bool random_start, int random_min_cells,
                        int random_max_cells,
                        Program *overall_best, Score *overall_best_score,
                        PromotionState *promotion) {
  rng_state = seed;
  if (!rng_state) rng_state = 1;

  Program current = *seed_program;
  if (random_start) {
    random_program(&current, ops, op_count, random_min_cells, random_max_cells);
  }
  Score current_score = score_program(&current, target);
  Program restart_best = current;
  Score restart_best_score = current_score;

  printf("\nrestart %d/%d seed=%llu start=%s\n", restart_index + 1,
         restart_count, (unsigned long long)seed,
         random_start ? "random" : "seed");
  print_program(&restart_best, &restart_best_score);

  if (restart_best_score.cost < overall_best_score->cost) {
    *overall_best = restart_best;
    *overall_best_score = restart_best_score;
    printf("new overall best at restart %d seed=%llu initial\n",
           restart_index + 1, (unsigned long long)seed);
    print_program(overall_best, overall_best_score);
    fflush(stdout);
  }

  double temperature = 20000.0;
  for (int i = 1; i <= iterations; i++) {
    Program mutated;
    mutate_program(&mutated, &current, ops, op_count, target->max_cells);
    Score mutated_score = score_program(&mutated, target);

    char where[128];
    snprintf(where, sizeof(where), "restart %d seed=%llu iteration %d",
             restart_index + 1, (unsigned long long)seed, i);
    if (try_promote_mastermind(&mutated, &mutated_score, promotion, where)) {
      current = mutated;
      current_score = mutated_score;
      restart_best = mutated;
      restart_best_score = mutated_score;
      if (mutated_score.cost < overall_best_score->cost) {
        *overall_best = mutated;
        *overall_best_score = mutated_score;
      }
    }

    double delta = mutated_score.cost - current_score.cost;
    bool accept = delta <= 0.0 || rng_double() < exp(-delta / temperature);
    if (accept) {
      current = mutated;
      current_score = mutated_score;
    }

    if (mutated_score.cost < restart_best_score.cost) {
      restart_best = mutated;
      restart_best_score = mutated_score;
      printf("\nnew restart best at restart %d seed=%llu iteration %d, temperature=%.6g\n",
             restart_index + 1, (unsigned long long)seed, i, temperature);
      print_program(&restart_best, &restart_best_score);
      fflush(stdout);
    }

    if (mutated_score.cost < overall_best_score->cost) {
      *overall_best = mutated;
      *overall_best_score = mutated_score;
      printf("\nnew overall best at restart %d seed=%llu iteration %d, temperature=%.6g\n",
             restart_index + 1, (unsigned long long)seed, i, temperature);
      print_program(overall_best, overall_best_score);
      fflush(stdout);
    }

    temperature *= 0.99998;
    if (temperature < 0.01) temperature = 0.01;
  }

  printf("\nrestart %d/%d best:\n", restart_index + 1, restart_count);
  print_program(&restart_best, &restart_best_score);
  fflush(stdout);
}

int main(int argc, char **argv) {
  const char *target_path = parse_string_arg(argc, argv, "--target", NULL);
  if (!target_path) {
    usage(argv[0]);
    return 2;
  }

  Target target;
  parse_target(target_path, &target);
  verbose_failures = has_flag(argc, argv, "--verbose");

  const int iterations = parse_int_arg(argc, argv, "--iterations", 300000);
  const char *seed_text = parse_string_arg(argc, argv, "--seed", NULL);
  const bool seed_from_millis = has_flag(argc, argv, "--seed-ms") || !seed_text;
  const uint64_t seed = seed_from_millis ? current_unix_millis() : parse_u64_value(seed_text);
  const int restarts = parse_int_arg(argc, argv, "--restarts", 1);
  const bool random_start = has_flag(argc, argv, "--random-start");
  int random_min_cells = parse_int_arg(argc, argv, "--random-min-cells", 1);
  int random_max_cells = parse_int_arg(argc, argv, "--random-max-cells", target.max_cells);
  PromotionState promotion = {0};
  promotion.enabled = has_flag(argc, argv, "--promote-mastermind");
  const char *promotion_log_path = parse_string_arg(argc, argv, "--promotion-log", NULL);
  if (promotion_log_path) {
    promotion.log_file = fopen(promotion_log_path, "a");
    if (!promotion.log_file) {
      perror(promotion_log_path);
      return 2;
    }
  }
  if (restarts <= 0) {
    fprintf(stderr, "--restarts must be >= 1\n");
    close_promotion_log(&promotion);
    return 2;
  }
  if (random_min_cells < 0 || random_max_cells < 0 ||
      random_min_cells > random_max_cells ||
      random_max_cells > target.max_cells) {
    fprintf(stderr, "--random-min-cells/--random-max-cells must satisfy 0 <= min <= max <= target max-cells\n");
    close_promotion_log(&promotion);
    return 2;
  }

  Operation ops[MAX_OPS];
  int op_count = full_operation_set(ops, MAX_OPS);

  Program seed_program;
  const char *candidate_text = parse_string_arg(argc, argv, "--candidate", NULL);
  if (candidate_text) {
    char candidate_buffer[MAX_LINE];
    snprintf(candidate_buffer, sizeof(candidate_buffer), "%s", candidate_text);
    uint16_t candidate_codes[MAX_CELLS];
    int candidate_len = parse_key_sequence(candidate_buffer, candidate_codes, MAX_CELLS, "<candidate>", 1);
    seed_from_codes(&seed_program, candidate_codes, candidate_len);
  } else {
    seed_from_codes(&seed_program, target.seed, target.seed_len);
  }
  Score seed_score = score_program(&seed_program, &target);
  Program overall_best = seed_program;
  Score overall_best_score = seed_score;
  promotion.best_cells = PROGRAM_SIZE + 1;
  promotion.promoted_program = seed_program;
  promotion.has_promoted = false;

  printf("target=%s seed=%llu seed-source=%s restarts=%d operations=%d iterations=%d max-cells=%d prefix=%d append=%d cases=%d random-start=%s random-cells=%d..%d promote-mastermind=%s promotion-log=%s\n",
         target.name, (unsigned long long)seed,
         seed_from_millis ? "unix-ms" : "manual", restarts, op_count,
         iterations, target.max_cells, target.prefix_len, target.append_len,
         target.case_count, random_start ? "yes" : "no",
         random_min_cells, random_max_cells,
         promotion.enabled ? "yes" : "no",
         promotion_log_path ? promotion_log_path : "-");
  printf("initial candidate:\n");
  print_program(&seed_program, &seed_score);
  if (promotion.enabled && try_promote_mastermind(&seed_program, &seed_score, &promotion, "initial candidate")) {
    seed_program = promotion.promoted_program;
    promotion.has_promoted = false;
  }

  if (has_flag(argc, argv, "--check") || iterations == 0) {
    int result = seed_score.exact == seed_score.total && seed_score.penalty == 0.0 ? 0 : 1;
    close_promotion_log(&promotion);
    return result;
  }

  for (int restart = 0; restart < restarts; restart++) {
    run_restart(&target, ops, op_count, &seed_program, restart, restarts,
                seed + (uint64_t)restart, iterations, random_start,
                random_min_cells, random_max_cells, &overall_best,
                &overall_best_score, &promotion);
    if (promotion.enabled && promotion.has_promoted) {
      seed_program = promotion.promoted_program;
      seed_score = score_program(&seed_program, &target);
      promotion.has_promoted = false;
      printf("\nnext restarts will use promoted seed with %d extract cells\n",
             seed_score.cells);
      fflush(stdout);
    }
  }

  printf("\noverall best:\n");
  print_program(&overall_best, &overall_best_score);
  if (promotion.enabled) {
    printf("promotion summary: tested=%d passed=%d best_extract_cells=%d\n",
           promotion.tested, promotion.passed, promotion.best_cells);
  }
  close_promotion_log(&promotion);
  return overall_best_score.exact == overall_best_score.total &&
         overall_best_score.penalty == 0.0 ? 0 : 1;
}
