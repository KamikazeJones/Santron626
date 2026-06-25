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
#define MAX_CASES 256
#define MAX_STEPS 8
#define MAX_CHECKS 32
#define MAX_KEYS 256
#define MAX_LINE 4096
#define MAX_AST_LINE 32768
#define MAX_TOP_FILES 16
#define MAX_TREE_NODES 128
#define MAX_SEQ_CHILDREN 4
#define EVAL_VARIANTS 3
#define MAX_EVALUATIONS 1024

typedef struct {
  int exact;
  int total;
  int matched_checks;
  int total_checks;
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
  int loop;
} StateSpec;

typedef enum {
  CHECK_X,
  CHECK_Y,
  CHECK_PC,
  CHECK_MEMORY,
  CHECK_PAUSE,
  CHECK_RUNNING,
  CHECK_PENDING_NONE,
  CHECK_PENDING_MEMORY_NONE,
  CHECK_PAREN,
  CHECK_EXPONENT,
  CHECK_LOOP
} CheckField;

typedef enum {
  CHECK_DISTANCE,
  CHECK_EXACT
} CheckMode;

typedef struct {
  CheckField field;
  int index;
  double expected;
  double weight;
  double tolerance;
  CheckMode mode;
  int line_no;
} TargetCheck;

typedef struct {
  StateSpec expect;
  uint16_t keys[MAX_KEYS];
  int key_count;
  TargetCheck checks[MAX_CHECKS];
  int check_count;
  int line_no;
} TargetStep;

typedef struct {
  StateSpec init;
  TargetStep steps[MAX_STEPS];
  int step_count;
  int line_no;
} TargetCase;

typedef enum {
  NUMERIC_RAW,
  NUMERIC_NORMALIZED
} NumericScoreMode;

typedef struct {
  char name[128];
  int max_cells;
  uint16_t prefix[MAX_CELLS];
  int prefix_len;
  uint16_t append[MAX_CELLS];
  int append_len;
  TargetCase cases[MAX_CASES];
  int case_count;
  bool modern_score;
  NumericScoreMode numeric_mode;
  double exact_weight;
  double invalid_loss;
  double timeout_loss;
} Target;

typedef enum {
  /* Anfaenglicher Displaywert beim Eintritt in den Kandidaten. TN_X gibt
     keine Taste aus und darf als lineare Eingabe nur einmal benutzt werden. */
  TN_X,
  /* Eine einzelne Ziffer 0..9. Mehrstellige Zahlen sind kein AST-Knoten,
     weil aufeinanderfolgende Konstanten bei der Santron-Eingabe verschmelzen. */
  TN_CONST,
  /* Speicherwert als Ausdruck: (RCL Mn), kompiliert zu RCL n. */
  TN_RCL,
  /* Reine Funktion mit einem Argument, zum Beispiel (SIN x) oder (LN x). */
  TN_UNARY,
  /* Bedingung z != 0. Kompiliert zu z X^2 +/-, damit das Ergebnis genau
     fuer z != 0 negativ und damit fuer SKP wahr ist. */
  TN_NONZERO,
  /* Ausdruck mit zwei Argumenten, zum Beispiel (+ a b) oder (/ a b). */
  TN_BINARY,
  /* Minimum zweier nebenwirkungsfreier Ausdruecke. Der zweite Ausdruck wird
     von der kompakten Santron-Routine zweimal ausgewertet. */
  TN_MIN,
  /* Speicheranweisung mit einem Ausdruck: (M+ Mn x), (STO Mn x), ... */
  TN_MEMOP,
  /* Bedingte Anweisung. Kind 0 ist ein Ausdruck, der bei "wahr" negativ
     werden muss. Kind 1 ist der Then-, Kind 2 optional der Else-Zweig. */
  TN_IF,
  /* Folge von Anweisungen. Nur an der Wurzel des Programms erlaubt. */
  TN_SEQ
} TreeKind;

typedef struct {
  TreeKind kind;
  int value;
  int child[MAX_SEQ_CHILDREN];
  int child_count;
} TreeNode;

typedef struct {
  TreeNode nodes[MAX_TREE_NODES];
  int node_count;
  int root;
} Tree;

static bool parse_ast_line(const char *line, Tree *tree);

typedef struct {
  Tree tree;
  uint16_t codes[MAX_CELLS];
  int code_len;
  Score score;
  /* Verlust pro Target-Fall und Initialzustandsvariante. Lexicase nutzt diese
     Einzelwerte, damit Spezialisten nicht in einer Gesamtsumme verschwinden. */
  float evaluation_loss[MAX_EVALUATIONS];
  int evaluation_count;
} Candidate;

typedef enum {
  ROLE_PROGRAM,
  ROLE_STATEMENT,
  ROLE_EXPRESSION
} NodeRole;

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

static bool nearly_equal(double a, double b) {
  if (isnan(a) && isnan(b)) return true;
  return fabs(a - b) <= 1e-8;
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

static void apply_numeric_assignment(StateSpec *spec, const char *name, double value,
                                     const char *path, int line_no) {
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

static TargetStep *add_step(TargetCase *tc, const char *path, int line_no) {
  if (tc->step_count >= MAX_STEPS) die_parse(path, line_no, "too many steps in case");
  TargetStep *step = &tc->steps[tc->step_count++];
  memset(step, 0, sizeof(*step));
  step->line_no = line_no;
  return step;
}

static TargetStep *current_step(Target *target, const char *path, int line_no) {
  TargetCase *tc = current_case(target, path, line_no);
  if (tc->step_count == 0) return add_step(tc, path, line_no);
  return &tc->steps[tc->step_count - 1];
}

static void parse_score_rule(Target *target, char *text,
                             const char *path, int line_no) {
  char *name = strtok(text, " \t\r\n");
  char *value = strtok(NULL, " \t\r\n");
  if (!name || !value || strtok(NULL, " \t\r\n")) {
    die_parse(path, line_no, "score requires exactly a name and value");
  }
  target->modern_score = true;
  if (strcmp(name, "numeric") == 0) {
    if (strcmp(value, "raw") == 0) target->numeric_mode = NUMERIC_RAW;
    else if (strcmp(value, "normalized") == 0) target->numeric_mode = NUMERIC_NORMALIZED;
    else die_parse(path, line_no, "score numeric must be raw or normalized");
    return;
  }

  char *end = NULL;
  double number = strtod(value, &end);
  if (end == value || *end != '\0' || number < 0.0) {
    die_parse(path, line_no, "score value must be a non-negative number");
  }
  if (strcmp(name, "exact-weight") == 0) target->exact_weight = number;
  else if (strcmp(name, "invalid") == 0) target->invalid_loss = number;
  else if (strcmp(name, "timeout") == 0) target->timeout_loss = number;
  else die_parse(path, line_no, "unknown score rule");
}

static void parse_check_field(const char *assignment, TargetCheck *check,
                              const char *path, int line_no) {
  char name[64];
  double value = 0.0;
  if (strcmp(assignment, "pending=none") == 0) {
    check->field = CHECK_PENDING_NONE;
    check->expected = 1.0;
    check->mode = CHECK_EXACT;
    return;
  }
  if (strcmp(assignment, "pending-memory=none") == 0) {
    check->field = CHECK_PENDING_MEMORY_NONE;
    check->expected = 1.0;
    check->mode = CHECK_EXACT;
    return;
  }
  if (strcmp(assignment, "loop=continue") == 0 ||
      strcmp(assignment, "loop=done") == 0) {
    check->field = CHECK_LOOP;
    check->expected = strcmp(assignment, "loop=continue") == 0 ? 1.0 : -1.0;
    check->mode = CHECK_EXACT;
    return;
  }
  if (!parse_assignment(assignment, name, sizeof(name), &value)) {
    die_parse(path, line_no, "invalid check assignment");
  }
  check->expected = value;
  check->mode = CHECK_DISTANCE;
  if (name[0] == 'M' && isdigit((unsigned char)name[1]) && name[2] == '\0') {
    check->field = CHECK_MEMORY;
    check->index = name[1] - '0';
  } else if (strcmp(name, "X") == 0) check->field = CHECK_X;
  else if (strcmp(name, "Y") == 0) check->field = CHECK_Y;
  else if (strcmp(name, "pc") == 0 || strcmp(name, "PC") == 0) {
    check->field = CHECK_PC;
    check->mode = CHECK_EXACT;
  } else if (strcmp(name, "pause") == 0 || strcmp(name, "paused") == 0) {
    check->field = CHECK_PAUSE;
    check->mode = CHECK_EXACT;
  } else if (strcmp(name, "running") == 0) {
    check->field = CHECK_RUNNING;
    check->mode = CHECK_EXACT;
  } else if (strcmp(name, "paren") == 0) {
    check->field = CHECK_PAREN;
    check->mode = CHECK_EXACT;
  } else if (strcmp(name, "exponent") == 0) {
    check->field = CHECK_EXPONENT;
    check->mode = CHECK_EXACT;
  } else {
    die_parse(path, line_no, "unknown check field");
  }
}

static void parse_check(TargetStep *step, char *text,
                        const char *path, int line_no) {
  if (step->check_count >= MAX_CHECKS) die_parse(path, line_no, "too many checks in step");
  TargetCheck *check = &step->checks[step->check_count++];
  memset(check, 0, sizeof(*check));
  check->weight = 1.0;
  check->line_no = line_no;

  char *assignment = strtok(text, " \t\r\n");
  if (!assignment) die_parse(path, line_no, "check requires an assignment");
  parse_check_field(assignment, check, path, line_no);

  for (char *tok = strtok(NULL, " \t\r\n"); tok; tok = strtok(NULL, " \t\r\n")) {
    char name[64];
    double value = 0.0;
    if (strcmp(tok, "mode=distance") == 0) check->mode = CHECK_DISTANCE;
    else if (strcmp(tok, "mode=exact") == 0) check->mode = CHECK_EXACT;
    else if (parse_assignment(tok, name, sizeof(name), &value) &&
             strcmp(name, "weight") == 0 && value >= 0.0) {
      check->weight = value;
    } else if (parse_assignment(tok, name, sizeof(name), &value) &&
               strcmp(name, "tolerance") == 0 && value >= 0.0) {
      check->tolerance = value;
    } else {
      die_parse(path, line_no, "invalid check option");
    }
  }
}

static void parse_target(const char *path, Target *target) {
  memset(target, 0, sizeof(*target));
  target->max_cells = 72;
  target->numeric_mode = NUMERIC_RAW;
  target->exact_weight = 0.0;
  target->invalid_loss = 100.0;
  target->timeout_loss = 1000.0;
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
    } else if (strcmp(keyword, "score") == 0) {
      parse_score_rule(target, rest, path, line_no);
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
      continue;
    } else if (strcmp(keyword, "case") == 0) {
      if (target->case_count >= MAX_CASES) die_parse(path, line_no, "too many cases");
      TargetCase *tc = &target->cases[target->case_count++];
      memset(tc, 0, sizeof(*tc));
      tc->line_no = line_no;
    } else if (strcmp(keyword, "step") == 0) {
      if (*rest != '\0') die_parse(path, line_no, "step takes no arguments");
      target->modern_score = true;
      add_step(current_case(target, path, line_no), path, line_no);
    } else if (strcmp(keyword, "init") == 0) {
      parse_state_spec(rest, &current_case(target, path, line_no)->init, false, path, line_no);
    } else if (strcmp(keyword, "expect") == 0) {
      parse_state_spec(rest, &current_step(target, path, line_no)->expect, true, path, line_no);
    } else if (strcmp(keyword, "check") == 0) {
      target->modern_score = true;
      parse_check(current_step(target, path, line_no), rest, path, line_no);
    } else if (strcmp(keyword, "keys") == 0) {
      TargetStep *step = current_step(target, path, line_no);
      step->key_count = parse_key_sequence(rest, step->keys, MAX_KEYS, path, line_no);
    } else {
      die_parse(path, line_no, "unknown directive");
    }
  }
  fclose(f);
  if (target->case_count == 0) {
    fprintf(stderr, "%s: no cases defined\n", path);
    exit(2);
  }
  for (int i = 0; i < target->case_count; i++) {
    if (target->cases[i].step_count == 0) {
      die_parse(path, target->cases[i].line_no, "case has no step, keys, expect, or check");
    }
  }
  if (target->name[0] == '\0') snprintf(target->name, sizeof(target->name), "unnamed");
}

static int state_spec_check_count(const StateSpec *spec) {
  int count = spec->has_x + spec->has_y + spec->has_pc +
              spec->has_pause + spec->has_running +
              spec->has_pending_none + spec->has_pending_memory_none +
              spec->has_paren + spec->has_exponent + spec->has_loop;
  for (int i = 0; i < MEMORY_SIZE; i++) count += spec->has_memory[i] ? 1 : 0;
  return count;
}

static int target_evaluation_count(const Target *target) {
  if (!target->modern_score) return target->case_count * EVAL_VARIANTS;
  int count = 0;
  for (int i = 0; i < target->case_count; i++) {
    for (int step = 0; step < target->cases[i].step_count; step++) {
      const TargetStep *target_step = &target->cases[i].steps[step];
      count += 1 + state_spec_check_count(&target_step->expect) +
               target_step->check_count;
    }
  }
  return count * EVAL_VARIANTS;
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

static bool run_step_keys(SantronState *s, const TargetStep *step) {
  bool timeout = false;
  for (int i = 0; i < step->key_count; i++) {
    int code = (int)step->keys[i];
    execute_key(s, code, false);
    if (code == KEY_RS) {
      int executed = run_program(s, 1000);
      if (executed >= 1000 && s->running) timeout = true;
    }
  }
  return timeout;
}

static bool check_numeric(double actual, double expected, double *error,
                          int *matched_checks, int *total_checks) {
  (*total_checks)++;
  if (nearly_equal(actual, expected)) {
    (*matched_checks)++;
    return true;
  }
  if (isfinite(actual) && isfinite(expected)) *error += fabs(actual - expected);
  else *error += 1e6;
  return false;
}

static bool check_bool(bool actual_ok, double penalty_value, double *penalty,
                       int *matched_checks, int *total_checks) {
  (*total_checks)++;
  if (actual_ok) {
    (*matched_checks)++;
    return true;
  }
  *penalty += penalty_value;
  return false;
}

static bool check_case(const SantronState *s, const StateSpec *expect,
                       double *error, double *penalty,
                       int *matched_checks, int *total_checks) {
  bool ok = true;
  if (expect->has_x) ok = check_numeric(s->x, expect->x, error, matched_checks, total_checks) && ok;
  if (expect->has_y) ok = check_numeric(s->y, expect->y, error, matched_checks, total_checks) && ok;
  if (expect->has_pc) ok = check_numeric((double)s->pc, (double)expect->pc, error, matched_checks, total_checks) && ok;
  for (int i = 0; i < MEMORY_SIZE; i++) {
    if (expect->has_memory[i]) ok = check_numeric(s->memory[i], expect->memory[i], error, matched_checks, total_checks) && ok;
  }
  if (expect->has_pause) ok = check_bool((s->paused ? 1 : 0) == expect->pause, 1000.0, penalty, matched_checks, total_checks) && ok;
  if (expect->has_running) ok = check_bool((s->running ? 1 : 0) == expect->running, 1000.0, penalty, matched_checks, total_checks) && ok;
  if (expect->has_pending_none) ok = check_bool(s->pending == OP_NONE, 1000.0, penalty, matched_checks, total_checks) && ok;
  if (expect->has_pending_memory_none) ok = check_bool(s->pending_memory < 0, 100.0, penalty, matched_checks, total_checks) && ok;
  if (expect->has_paren) ok = check_bool(s->paren_depth == expect->paren, 1000.0, penalty, matched_checks, total_checks) && ok;
  if (expect->has_exponent) ok = check_bool((s->exponent_entry ? 1 : 0) == expect->exponent, 100.0, penalty, matched_checks, total_checks) && ok;
  if (expect->has_loop) {
    bool loop_ok = expect->loop > 0 ? s->x >= 0.0 : s->x < 0.0;
    ok = check_bool(loop_ok, 1000.0, penalty, matched_checks, total_checks) && ok;
  }
  return ok;
}

static double check_actual_value(const SantronState *s,
                                 const TargetCheck *check) {
  switch (check->field) {
    case CHECK_X: return s->x;
    case CHECK_Y: return s->y;
    case CHECK_PC: return (double)s->pc;
    case CHECK_MEMORY: return s->memory[check->index];
    case CHECK_PAUSE: return s->paused ? 1.0 : 0.0;
    case CHECK_RUNNING: return s->running ? 1.0 : 0.0;
    case CHECK_PENDING_NONE: return s->pending == OP_NONE ? 1.0 : 0.0;
    case CHECK_PENDING_MEMORY_NONE: return s->pending_memory < 0 ? 1.0 : 0.0;
    case CHECK_PAREN: return (double)s->paren_depth;
    case CHECK_EXPONENT: return s->exponent_entry ? 1.0 : 0.0;
    case CHECK_LOOP:
      return check->expected > 0.0 ? (s->x >= 0.0 ? 1.0 : 0.0)
                                   : (s->x < 0.0 ? -1.0 : 0.0);
  }
  return NAN;
}

static double modern_check_loss(const SantronState *s,
                                const TargetCheck *check,
                                const Target *target,
                                bool *matched, double *distance) {
  double actual = check_actual_value(s, check);
  double difference = INFINITY;
  if (isfinite(actual) && isfinite(check->expected)) {
    difference = fabs(actual - check->expected);
  }
  *matched = difference <= check->tolerance ||
             (check->tolerance == 0.0 && nearly_equal(actual, check->expected));
  *distance = 0.0;
  if (*matched) return 0.0;
  if (check->mode == CHECK_EXACT) return check->weight;
  if (!isfinite(difference)) return check->weight * target->invalid_loss;
  if (target->numeric_mode == NUMERIC_NORMALIZED) {
    difference /= 1.0 + fabs(check->expected);
  }
  *distance = difference;
  return check->weight * (target->exact_weight + difference);
}

static bool append_evaluation(float *evaluation_loss, int *evaluation,
                              double loss) {
  if (*evaluation >= MAX_EVALUATIONS) return false;
  evaluation_loss[(*evaluation)++] = (float)loss;
  return true;
}

static bool apply_modern_check(const SantronState *s, const TargetCheck *check,
                               const Target *target, Score *score,
                               float *evaluation_loss, int *evaluation) {
  bool matched = false;
  double distance = 0.0;
  double loss = modern_check_loss(s, check, target, &matched, &distance);
  score->total_checks++;
  if (matched) score->matched_checks++;
  if (check->mode == CHECK_DISTANCE) score->error += distance;
  else score->penalty += loss;
  score->cost += loss;
  return append_evaluation(evaluation_loss, evaluation, loss);
}

static bool apply_state_spec_checks(const SantronState *s,
                                    const StateSpec *expect,
                                    const Target *target, Score *score,
                                    float *evaluation_loss, int *evaluation,
                                    bool *step_ok) {
  TargetCheck check = {
    .weight = 1.0,
    .tolerance = 0.0,
    .mode = CHECK_DISTANCE
  };
#define APPLY_NUMERIC(field_name, expected_value) do { \
    check.field = (field_name); \
    check.expected = (expected_value); \
    check.index = 0; \
    check.mode = CHECK_DISTANCE; \
    if (!apply_modern_check(s, &check, target, score, evaluation_loss, evaluation)) return false; \
    if (evaluation_loss[*evaluation - 1] != 0.0f) *step_ok = false; \
  } while (0)
#define APPLY_EXACT(field_name, expected_value) do { \
    check.field = (field_name); \
    check.expected = (expected_value); \
    check.index = 0; \
    check.mode = CHECK_EXACT; \
    if (!apply_modern_check(s, &check, target, score, evaluation_loss, evaluation)) return false; \
    if (evaluation_loss[*evaluation - 1] != 0.0f) *step_ok = false; \
  } while (0)

  if (expect->has_x) APPLY_NUMERIC(CHECK_X, expect->x);
  if (expect->has_y) APPLY_NUMERIC(CHECK_Y, expect->y);
  if (expect->has_pc) APPLY_EXACT(CHECK_PC, expect->pc);
  for (int i = 0; i < MEMORY_SIZE; i++) {
    if (!expect->has_memory[i]) continue;
    check.field = CHECK_MEMORY;
    check.index = i;
    check.expected = expect->memory[i];
    check.mode = CHECK_DISTANCE;
    if (!apply_modern_check(s, &check, target, score, evaluation_loss, evaluation)) return false;
    if (evaluation_loss[*evaluation - 1] != 0.0f) *step_ok = false;
  }
  if (expect->has_pause) APPLY_EXACT(CHECK_PAUSE, expect->pause);
  if (expect->has_running) APPLY_EXACT(CHECK_RUNNING, expect->running);
  if (expect->has_pending_none) APPLY_EXACT(CHECK_PENDING_NONE, 1.0);
  if (expect->has_pending_memory_none) APPLY_EXACT(CHECK_PENDING_MEMORY_NONE, 1.0);
  if (expect->has_paren) APPLY_EXACT(CHECK_PAREN, expect->paren);
  if (expect->has_exponent) APPLY_EXACT(CHECK_EXPONENT, expect->exponent);
  if (expect->has_loop) APPLY_EXACT(CHECK_LOOP, expect->loop);
#undef APPLY_NUMERIC
#undef APPLY_EXACT
  return true;
}

static Score score_codes_modern(const uint16_t *codes, int len,
                                const Target *target,
                                float *evaluation_loss,
                                int *evaluation_count) {
  Score score = {0};
  score.cells = len;
  int evaluation = 0;

  for (int i = 0; i < target->case_count; i++) {
    for (int variant = 0; variant < EVAL_VARIANTS; variant++) {
      SantronState s;
      init_state(&s);
      fill_unspecified_state(&s, &target->cases[i].init, variant);
      load_candidate(&s, codes, len, target);
      apply_init(&s, &target->cases[i].init);
      for (int step_index = 0;
           step_index < target->cases[i].step_count;
           step_index++) {
        const TargetStep *step = &target->cases[i].steps[step_index];
        bool step_ok = true;
        bool timeout = run_step_keys(&s, step);
        double timeout_loss = timeout ? target->timeout_loss : 0.0;
        score.total_checks++;
        if (!timeout) score.matched_checks++;
        else {
          score.penalty += timeout_loss;
          score.cost += timeout_loss;
          step_ok = false;
        }
        if (!append_evaluation(evaluation_loss, &evaluation, timeout_loss)) {
          score.cost = INFINITY;
          *evaluation_count = evaluation;
          return score;
        }
        if (!apply_state_spec_checks(&s, &step->expect, target, &score,
                                     evaluation_loss, &evaluation, &step_ok)) {
          score.cost = INFINITY;
          *evaluation_count = evaluation;
          return score;
        }
        for (int check_index = 0; check_index < step->check_count; check_index++) {
          if (!apply_modern_check(&s, &step->checks[check_index], target,
                                  &score, evaluation_loss, &evaluation)) {
            score.cost = INFINITY;
            *evaluation_count = evaluation;
            return score;
          }
          if (evaluation_loss[evaluation - 1] != 0.0f) step_ok = false;
        }
        score.total++;
        if (step_ok) score.exact++;
      }
    }
  }
  *evaluation_count = evaluation;
  bool complete = score.exact == score.total &&
                  score.matched_checks == score.total_checks;
  if (complete) score.cost += (double)score.cells;
  return score;
}

static Score score_codes_legacy(const uint16_t *codes, int len,
                                const Target *target,
                                float *evaluation_loss,
                                int *evaluation_count) {
  Score score = {0};
  score.cells = len;
  score.total = target->case_count * EVAL_VARIANTS;
  int evaluation = 0;

  for (int i = 0; i < target->case_count; i++) {
    for (int variant = 0; variant < EVAL_VARIANTS; variant++) {
      SantronState s;
      init_state(&s);
      fill_unspecified_state(&s, &target->cases[i].init, variant);
      load_candidate(&s, codes, len, target);
      apply_init(&s, &target->cases[i].init);
      const TargetStep *step = &target->cases[i].steps[0];
      run_step_keys(&s, step);
      double error = 0.0;
      double penalty = 0.0;
      int matched_checks = 0;
      int total_checks = 0;
      bool ok = check_case(&s, &step->expect, &error, &penalty,
                           &matched_checks, &total_checks);
      if (ok) score.exact++;
      score.matched_checks += matched_checks;
      score.total_checks += total_checks;
      score.error += error;
      score.penalty += penalty;
      int missed = total_checks - matched_checks;
      double loss = (double)missed * 10000.0 +
                    fmin(error, 100000.0) * 0.01 +
                    fmin(penalty, 100000.0) * 0.1;
      if (!append_evaluation(evaluation_loss, &evaluation, loss)) {
        score.cost = INFINITY;
        *evaluation_count = evaluation;
        return score;
      }
    }
  }
  *evaluation_count = evaluation;

  int missed_checks = score.total_checks - score.matched_checks;
  double capped_error = fmin(score.error, 100000.0);
  double capped_penalty = fmin(score.penalty, 100000.0);
  bool complete = score.exact == score.total &&
                  score.penalty == 0.0 &&
                  score.matched_checks == score.total_checks;
  score.cost = (double)missed_checks * 10000.0 +
               capped_error * 0.01 +
               capped_penalty * 0.1 +
               (complete ? (double)score.cells : 0.0);
  return score;
}

static Score score_codes(const uint16_t *codes, int len, const Target *target,
                         float *evaluation_loss, int *evaluation_count) {
  return target->modern_score
           ? score_codes_modern(codes, len, target,
                                evaluation_loss, evaluation_count)
           : score_codes_legacy(codes, len, target,
                                evaluation_loss, evaluation_count);
}

static int tree_add(Tree *tree, TreeKind kind, int value) {
  if (tree->node_count >= MAX_TREE_NODES) return -1;
  int idx = tree->node_count++;
  TreeNode *node = &tree->nodes[idx];
  memset(node, 0, sizeof(*node));
  node->kind = kind;
  node->value = value;
  return idx;
}

static int random_leaf(Tree *tree) {
  int choice = rng_int(10);
  if (choice == 0) return tree_add(tree, TN_X, 0);
  if (choice < 5) return tree_add(tree, TN_CONST, rng_int(10));
  return tree_add(tree, TN_RCL, rng_int(MEMORY_SIZE));
}

static int random_unary_key(void) {
  static const int keys[] = {
    KEY_SIGN, KEY_INV, KEY_SQR, KEY_SQRT, KEY_LN, KEY_LOG, KEY_EX, KEY_TENX,
    KEY_SIN, KEY_COS, KEY_TAN
  };
  return keys[rng_int((int)(sizeof(keys) / sizeof(keys[0])))];
}

static int random_binary_key(void) {
  static const int keys[] = {KEY_ADD, KEY_SUB, KEY_MUL, KEY_DIV};
  return keys[rng_int((int)(sizeof(keys) / sizeof(keys[0])))];
}

static int random_memop_key(void) {
  static const int keys[] = {KEY_STO, KEY_MPLUS, KEY_MMINUS, KEY_MULMEM, KEY_DIVMEM};
  return keys[rng_int((int)(sizeof(keys) / sizeof(keys[0])))];
}

static int random_expression(Tree *tree, int depth);
static int random_statement(Tree *tree, int depth);
static int random_pure_expression(Tree *tree, int depth, bool allow_x);

static int random_pure_leaf(Tree *tree, bool allow_x) {
  int choice = rng_int(allow_x ? 10 : 9);
  if (allow_x && choice == 0) return tree_add(tree, TN_X, 0);
  if (choice < (allow_x ? 5 : 4)) {
    return tree_add(tree, TN_CONST, rng_int(10));
  }
  return tree_add(tree, TN_RCL, rng_int(MEMORY_SIZE));
}

/* MIN muss seinen rechten Operanden zweimal ausgeben. Dieser Generator baut
   deshalb nur reine Ausdruecke ohne Speicheroperationen oder Kontrollfluss.
   Im rechten MIN-Operanden ist auch TN_X verboten. */
static int random_pure_expression(Tree *tree, int depth, bool allow_x) {
  if (depth <= 0 || tree->node_count > MAX_TREE_NODES - 8) {
    return random_pure_leaf(tree, allow_x);
  }
  int choice = rng_int(100);
  if (choice < 35) return random_pure_leaf(tree, allow_x);
  if (choice < 60) {
    int idx = tree_add(tree, TN_UNARY, random_unary_key());
    if (idx < 0) return random_pure_leaf(tree, allow_x);
    tree->nodes[idx].child[0] =
      random_pure_expression(tree, depth - 1, allow_x);
    tree->nodes[idx].child_count = 1;
    return idx;
  }

  TreeKind kind = choice < 90 ? TN_BINARY : TN_MIN;
  int idx = tree_add(tree, kind,
                     kind == TN_BINARY ? random_binary_key() : 0);
  if (idx < 0) return random_pure_leaf(tree, allow_x);
  tree->nodes[idx].child[0] =
    random_pure_expression(tree, depth - 1, allow_x);
  tree->nodes[idx].child[1] =
    random_pure_expression(tree, depth - 1, false);
  tree->nodes[idx].child_count = 2;
  return idx;
}

static int random_condition(Tree *tree, int depth) {
  int idx = tree_add(tree, TN_NONZERO, 0);
  if (idx < 0) return random_expression(tree, depth);
  tree->nodes[idx].child[0] =
    random_expression(tree, depth > 0 ? depth - 1 : 0);
  tree->nodes[idx].child_count = 1;
  return idx;
}

/* Eine Speicheroperation wertet zuerst ihren Ausdruck aus und fuehrt danach
   die zweistellige Santron-Anweisung aus, zum Beispiel "expr M+ 7". */
static int random_memop(Tree *tree, int depth) {
  int idx = tree_add(tree, TN_MEMOP, random_memop_key());
  if (idx < 0) return random_leaf(tree);
  tree->nodes[idx].value = tree->nodes[idx].value * 10 + rng_int(MEMORY_SIZE);
  tree->nodes[idx].child[0] = random_expression(tree, depth > 0 ? depth - 1 : 0);
  tree->nodes[idx].child_count = 1;
  return idx;
}

/* Erzeugt eine bedingte Anweisung. Die Bedingung kompiliert zu einem
   negativen Displaywert, wenn sie wahr ist; SKP kann sie damit direkt
   auswerten. Die Zweige sind Statements und keine Werte. */
static int random_if(Tree *tree, int depth) {
  int idx = tree_add(tree, TN_IF, 0);
  if (idx < 0) return random_memop(tree, depth);
  bool has_else = rng_int(100) < 35;
  tree->nodes[idx].child_count = has_else ? 3 : 2;
  tree->nodes[idx].child[0] =
    random_condition(tree, depth > 0 ? depth - 1 : 0);
  tree->nodes[idx].child[1] =
    random_memop(tree, depth > 0 ? depth - 1 : 0);
  if (has_else) {
    tree->nodes[idx].child[2] =
      random_memop(tree, depth > 0 ? depth - 1 : 0);
  }
  return idx;
}

static int random_statement(Tree *tree, int depth) {
  if (depth > 0 && rng_int(100) < 30) return random_if(tree, depth);
  return random_memop(tree, depth);
}

/* Baut genau einen Ausdruck. TN_SEQ ist hier absichtlich ausgeschlossen:
   Eine Anweisungsfolge darf weder Operand einer Rechnung noch Argument einer
   Funktion sein. */
static int random_expression(Tree *tree, int depth) {
  if (depth <= 0 || tree->node_count > MAX_TREE_NODES - 8) return random_leaf(tree);
  int choice = rng_int(100);
  if (choice < 30) {
    return random_leaf(tree);
  } else if (choice < 55) {
    int idx = tree_add(tree, TN_UNARY, random_unary_key());
    if (idx < 0) return random_leaf(tree);
    tree->nodes[idx].child[0] = random_expression(tree, depth - 1);
    tree->nodes[idx].child_count = 1;
    return idx;
  } else if (choice < 85) {
    TreeKind kind = rng_int(100) < 15 ? TN_MIN : TN_BINARY;
    int idx = tree_add(tree, kind,
                       kind == TN_BINARY ? random_binary_key() : 0);
    if (idx < 0) return random_leaf(tree);
    if (kind == TN_MIN) {
      tree->nodes[idx].child[0] =
        random_pure_expression(tree, depth - 1, true);
      tree->nodes[idx].child[1] =
        random_pure_expression(tree, depth - 1, false);
    } else {
      tree->nodes[idx].child[0] = random_expression(tree, depth - 1);
      tree->nodes[idx].child[1] = random_expression(tree, depth - 1);
    }
    tree->nodes[idx].child_count = 2;
    return idx;
  } else {
    return random_memop(tree, depth);
  }
}

/* Ein vollstaendiges Programm ist entweder ein einzelner Ausdruck oder:

     (seq Statement ... Statement Abschlussausdruck)

   Beliebige Ausdrucksfolgen waeren auf dem Taschenrechner mehrdeutig:
   (seq 4 5 4711) kompiliert zu der Zahl 454711, und (seq 5 M7) verwirft
   die 5 sofort wieder. Solche Baeume werden deshalb gar nicht erzeugt. */
static int random_program(Tree *tree, int depth) {
  if (depth > 0 && rng_int(100) < 20) {
    int idx = tree_add(tree, TN_SEQ, 0);
    if (idx < 0) return random_leaf(tree);
    int count = 2 + rng_int(MAX_SEQ_CHILDREN - 1);
    tree->nodes[idx].child_count = count;
    for (int i = 0; i < count - 1; i++) {
      tree->nodes[idx].child[i] = random_statement(tree, depth - 1);
    }
    tree->nodes[idx].child[count - 1] = random_expression(tree, depth - 1);
    return idx;
  }
  return random_expression(tree, depth);
}

static Tree random_tree(int depth) {
  Tree tree;
  memset(&tree, 0, sizeof(tree));
  tree.root = random_program(&tree, depth);
  return tree;
}

static bool tree_is_pure_expression(const Tree *tree, int idx) {
  if (idx < 0 || idx >= tree->node_count) return false;
  const TreeNode *node = &tree->nodes[idx];
  switch (node->kind) {
    case TN_X:
    case TN_CONST:
    case TN_RCL:
      return true;
    case TN_UNARY:
    case TN_NONZERO:
      return tree_is_pure_expression(tree, node->child[0]);
    case TN_BINARY:
    case TN_MIN:
      return tree_is_pure_expression(tree, node->child[0]) &&
             tree_is_pure_expression(tree, node->child[1]);
    case TN_MEMOP:
    case TN_IF:
    case TN_SEQ:
      return false;
  }
  return false;
}

static bool tree_contains_x(const Tree *tree, int idx) {
  if (idx < 0 || idx >= tree->node_count) return false;
  const TreeNode *node = &tree->nodes[idx];
  if (node->kind == TN_X) return true;
  for (int i = 0; i < node->child_count; i++) {
    if (tree_contains_x(tree, node->child[i])) return true;
  }
  return false;
}

static bool tree_expression_is_valid(const Tree *tree, int idx);
static bool tree_statement_is_valid(const Tree *tree, int idx);

/* Ein Ausdruck liefert einen Displaywert. Speicheroperationen duerfen darin
   vorkommen, weil der Santron nach STO/M+/... den Displaywert beibehaelt.
   IF und SEQ sind dagegen reine Kontrollfluss-/Anweisungsknoten. */
static bool tree_expression_is_valid(const Tree *tree, int idx) {
  if (idx < 0 || idx >= tree->node_count) return false;
  const TreeNode *node = &tree->nodes[idx];
  switch (node->kind) {
    case TN_X:
    case TN_CONST:
    case TN_RCL:
      return node->child_count == 0;
    case TN_UNARY:
      return node->child_count == 1 &&
             tree_expression_is_valid(tree, node->child[0]);
    case TN_NONZERO:
      return node->child_count == 1 &&
             tree_expression_is_valid(tree, node->child[0]);
    case TN_BINARY:
      return node->child_count == 2 &&
             tree_expression_is_valid(tree, node->child[0]) &&
             tree_expression_is_valid(tree, node->child[1]);
    case TN_MIN:
      return node->child_count == 2 &&
             tree_expression_is_valid(tree, node->child[0]) &&
             tree_expression_is_valid(tree, node->child[1]) &&
             tree_is_pure_expression(tree, node->child[0]) &&
             tree_is_pure_expression(tree, node->child[1]) &&
             !tree_contains_x(tree, node->child[1]);
    case TN_MEMOP:
      return node->child_count == 1 &&
             tree_expression_is_valid(tree, node->child[0]);
    case TN_IF:
    case TN_SEQ:
      return false;
  }
  return false;
}

/* Statements veraendern Zustand oder Kontrollfluss. Eine IF-Bedingung muss
   derzeit explizit der einstellige Vergleich (!=0 expression) sein. */
static bool tree_statement_is_valid(const Tree *tree, int idx) {
  if (idx < 0 || idx >= tree->node_count) return false;
  const TreeNode *node = &tree->nodes[idx];
  if (node->kind == TN_MEMOP) {
    return node->child_count == 1 &&
           tree_expression_is_valid(tree, node->child[0]);
  }
  if (node->kind != TN_IF ||
      (node->child_count != 2 && node->child_count != 3)) {
    return false;
  }
  int condition = node->child[0];
  if (condition < 0 || condition >= tree->node_count ||
      tree->nodes[condition].kind != TN_NONZERO ||
      !tree_expression_is_valid(tree, condition)) {
    return false;
  }
  for (int i = 1; i < node->child_count; i++) {
    if (!tree_statement_is_valid(tree, node->child[i])) return false;
  }
  return true;
}

/* Prueft die Programmgrammatik unabhaengig vom Zufallsgenerator. Das ist
   erforderlich, weil Mutation und Crossover Teilbaeume versetzen. */
static bool tree_program_is_valid(const Tree *tree) {
  if (tree->root < 0 || tree->root >= tree->node_count) return false;
  const TreeNode *root = &tree->nodes[tree->root];
  if (root->kind != TN_SEQ) {
    return tree_expression_is_valid(tree, tree->root) ||
           tree_statement_is_valid(tree, tree->root);
  }
  if (root->child_count < 2) return false;
  for (int i = 0; i < root->child_count - 1; i++) {
    if (!tree_statement_is_valid(tree, root->child[i])) return false;
  }
  return tree_expression_is_valid(tree, root->child[root->child_count - 1]);
}

/* TN_X ist keine normale Variable. Es bezeichnet den Displaywert, der beim
   Eintritt in den Kandidaten bereits vorhanden ist. Sobald irgendeine Taste
   ausgegeben wurde, kann dieser urspruengliche Wert nicht mehr kostenlos
   referenziert werden.

   Die Traversierung folgt derselben Auswertungsreihenfolge wie der Compiler.
   Sie akzeptiert TN_X deshalb nur als allererstes ausgewertetes Blatt und
   hoechstens einmal im gesamten Baum. */
static bool tree_x_usage_is_valid_node(const Tree *tree, int idx,
                                       bool *input_available, int *x_count) {
  if (idx < 0 || idx >= tree->node_count) return false;
  const TreeNode *node = &tree->nodes[idx];

  if (node->kind == TN_X) {
    (*x_count)++;
    if (*x_count > 1 || !*input_available) return false;
    *input_available = false;
    return true;
  }

  switch (node->kind) {
    case TN_CONST:
    case TN_RCL:
      *input_available = false;
      return true;
    case TN_UNARY:
      if (!tree_x_usage_is_valid_node(tree, node->child[0],
                                      input_available, x_count)) {
        return false;
      }
      *input_available = false;
      return true;
    case TN_NONZERO:
      if (!tree_x_usage_is_valid_node(tree, node->child[0],
                                      input_available, x_count)) {
        return false;
      }
      *input_available = false;
      return true;
    case TN_BINARY:
      if (!tree_x_usage_is_valid_node(tree, node->child[0],
                                      input_available, x_count)) {
        return false;
      }
      *input_available = false; /* Der Operator ist eine ausgegebene Taste. */
      return tree_x_usage_is_valid_node(tree, node->child[1],
                                        input_available, x_count);
    case TN_MIN:
      if (!tree_x_usage_is_valid_node(tree, node->child[0],
                                      input_available, x_count)) {
        return false;
      }
      *input_available = false;
      return tree_x_usage_is_valid_node(tree, node->child[1],
                                        input_available, x_count);
    case TN_MEMOP:
      if (!tree_x_usage_is_valid_node(tree, node->child[0],
                                      input_available, x_count)) {
        return false;
      }
      *input_available = false;
      return true;
    case TN_IF:
      /* Die Bedingung wird immer ausgewertet und verbraucht damit einen
         eventuell vorhandenen Eingangswert. Beide Zweige beginnen erst
         danach; TN_X ist dort folglich nicht mehr zulaessig. */
      if (!tree_x_usage_is_valid_node(tree, node->child[0],
                                      input_available, x_count)) {
        return false;
      }
      *input_available = false;
      for (int i = 1; i < node->child_count; i++) {
        if (!tree_x_usage_is_valid_node(tree, node->child[i],
                                        input_available, x_count)) {
          return false;
        }
      }
      return true;
    case TN_SEQ:
      for (int i = 0; i < node->child_count; i++) {
        if (!tree_x_usage_is_valid_node(tree, node->child[i],
                                        input_available, x_count)) {
          return false;
        }
        *input_available = false;
      }
      return true;
    case TN_X:
      break;
  }
  return false;
}

static bool tree_is_valid(const Tree *tree) {
  if (!tree_program_is_valid(tree)) return false;
  bool input_available = true;
  int x_count = 0;
  return tree_x_usage_is_valid_node(tree, tree->root,
                                    &input_available, &x_count);
}

static int clone_subtree(Tree *dst, const Tree *src, int src_idx) {
  if (src_idx < 0 || src_idx >= src->node_count) return -1;
  const TreeNode *src_node = &src->nodes[src_idx];
  int idx = tree_add(dst, src_node->kind, src_node->value);
  if (idx < 0) return -1;
  dst->nodes[idx].child_count = src_node->child_count;
  for (int i = 0; i < src_node->child_count; i++) {
    dst->nodes[idx].child[i] = clone_subtree(dst, src, src_node->child[i]);
  }
  return idx;
}

static int clone_replace_subtree(Tree *dst, const Tree *base, int base_idx,
                                 int replace_idx, const Tree *donor, int donor_idx) {
  if (base_idx == replace_idx) return clone_subtree(dst, donor, donor_idx);
  const TreeNode *base_node = &base->nodes[base_idx];
  int idx = tree_add(dst, base_node->kind, base_node->value);
  if (idx < 0) return -1;
  dst->nodes[idx].child_count = base_node->child_count;
  for (int i = 0; i < base_node->child_count; i++) {
    dst->nodes[idx].child[i] = clone_replace_subtree(dst, base, base_node->child[i],
                                                     replace_idx, donor, donor_idx);
  }
  return idx;
}

/* Bestimmt aus der Elternposition, welche Art Teilbaum an einer Stelle
   eingesetzt werden darf. Der Knotentyp allein reicht nicht: TN_MEMOP kann
   beispielsweise Ausdruck oder Statement sein. */
static NodeRole node_role(const Tree *tree, int idx) {
  if (idx == tree->root) return ROLE_PROGRAM;
  for (int parent_idx = 0; parent_idx < tree->node_count; parent_idx++) {
    const TreeNode *parent = &tree->nodes[parent_idx];
    for (int child_pos = 0; child_pos < parent->child_count; child_pos++) {
      if (parent->child[child_pos] != idx) continue;
      if (parent->kind == TN_IF && child_pos > 0) return ROLE_STATEMENT;
      if (parent->kind == TN_SEQ &&
          child_pos < parent->child_count - 1) {
        return ROLE_STATEMENT;
      }
      return ROLE_EXPRESSION;
    }
  }
  return ROLE_EXPRESSION;
}

static int different_value(int current, int limit) {
  if (limit <= 1) return current;
  int value = rng_int(limit - 1);
  return value >= current ? value + 1 : value;
}

/* Aendert nur einen Parameter eines vorhandenen Knotens. Diese Mutation ist
   fuer bereits gute Programme wichtig: Sie kann beispielsweise M7 durch M2,
   eine Konstante oder einen Operator ersetzen, ohne den umgebenden
   funktionierenden Teilbaum neu zu erzeugen. */
static Tree point_mutate_tree(const Tree *src) {
  int mutable_nodes[MAX_TREE_NODES];
  int mutable_count = 0;
  for (int i = 0; i < src->node_count; i++) {
    TreeKind kind = src->nodes[i].kind;
    if (kind == TN_CONST || kind == TN_RCL || kind == TN_UNARY ||
        kind == TN_BINARY || kind == TN_MEMOP || kind == TN_MIN) {
      mutable_nodes[mutable_count++] = i;
    }
  }
  if (mutable_count == 0) return *src;

  for (int attempt = 0; attempt < 8; attempt++) {
    Tree dst = *src;
    TreeNode *node = &dst.nodes[mutable_nodes[rng_int(mutable_count)]];
    switch (node->kind) {
      case TN_CONST:
        node->value = different_value(node->value % 10, 10);
        break;
      case TN_RCL:
        node->value = different_value(node->value % MEMORY_SIZE, MEMORY_SIZE);
        break;
      case TN_UNARY: {
        int old_key = node->value;
        do node->value = random_unary_key(); while (node->value == old_key);
        break;
      }
      case TN_BINARY: {
        if (rng_int(4) == 0) {
          int tmp = node->child[0];
          node->child[0] = node->child[1];
          node->child[1] = tmp;
        } else {
          int old_key = node->value;
          do node->value = random_binary_key(); while (node->value == old_key);
        }
        break;
      }
      case TN_MIN: {
        int tmp = node->child[0];
        node->child[0] = node->child[1];
        node->child[1] = tmp;
        break;
      }
      case TN_MEMOP: {
        int op = node->value / 10;
        int memory = node->value % 10;
        if (rng_int(2) == 0) {
          int old_op = op;
          do op = random_memop_key(); while (op == old_op);
        } else {
          memory = different_value(memory, MEMORY_SIZE);
        }
        node->value = op * 10 + memory;
        break;
      }
      case TN_X:
      case TN_NONZERO:
      case TN_IF:
      case TN_SEQ:
        break;
    }
    if (tree_is_valid(&dst)) return dst;
  }
  return *src;
}

static Tree mutate_tree(const Tree *src, int depth) {
  for (int attempt = 0; attempt < 8; attempt++) {
    int replace_idx = rng_int(src->node_count);
    Tree donor;
    memset(&donor, 0, sizeof(donor));
    int donor_depth = depth > 1 ? depth - 1 : 1;
    switch (node_role(src, replace_idx)) {
      case ROLE_PROGRAM:
        donor.root = random_program(&donor, donor_depth);
        break;
      case ROLE_STATEMENT:
        donor.root = random_statement(&donor, donor_depth);
        break;
      case ROLE_EXPRESSION:
        donor.root = random_expression(&donor, donor_depth);
        break;
    }
    Tree dst;
    memset(&dst, 0, sizeof(dst));
    dst.root = clone_replace_subtree(&dst, src, src->root, replace_idx,
                                     &donor, donor.root);
    if (dst.root >= 0 && tree_is_valid(&dst)) return dst;
  }
  return *src;
}

static Tree crossover_tree(const Tree *a, const Tree *b) {
  for (int attempt = 0; attempt < 8; attempt++) {
    Tree dst;
    memset(&dst, 0, sizeof(dst));
    int replace_idx = rng_int(a->node_count);
    NodeRole wanted_role = node_role(a, replace_idx);
    int donor_idx = -1;
    if (wanted_role == ROLE_PROGRAM) {
      donor_idx = b->root;
    } else {
      /* Nur Teilbaeume derselben syntaktischen Rolle kreuzen. Damit kann ein
         IF-Statement nicht als Operand einer Rechnung eingesetzt werden. */
      int candidates[MAX_TREE_NODES];
      int candidate_count = 0;
      for (int i = 0; i < b->node_count; i++) {
        if (node_role(b, i) == wanted_role) {
          candidates[candidate_count++] = i;
        }
      }
      if (candidate_count == 0) continue;
      donor_idx = candidates[rng_int(candidate_count)];
    }
    dst.root = clone_replace_subtree(&dst, a, a->root, replace_idx, b, donor_idx);
    if (dst.root >= 0 && tree_is_valid(&dst)) return dst;
  }
  return *a;
}

static bool emit_key(uint16_t *codes, int *len, int max_len, int key) {
  if (*len >= max_len) return false;
  codes[(*len)++] = (uint16_t)key;
  return true;
}

static bool patch_address(uint16_t *codes, int address_pos, int address) {
  if (address < 0 || address >= PROGRAM_SIZE) return false;
  codes[address_pos] = (uint16_t)(KEY_DIGIT0 + address / 10);
  codes[address_pos + 1] = (uint16_t)(KEY_DIGIT0 + address % 10);
  return true;
}

static bool node_contains_binary(const Tree *tree, int idx) {
  if (idx < 0 || idx >= tree->node_count) return false;
  const TreeNode *node = &tree->nodes[idx];
  if (node->kind == TN_BINARY || node->kind == TN_MIN) return true;
  for (int i = 0; i < node->child_count; i++) {
    if (node_contains_binary(tree, node->child[i])) return true;
  }
  return false;
}

/* Uebersetzt einen AST in tatsaechlich zu drueckende Santron-Tasten.

   Der Santron verwendet eine laufende binaere Operation statt der ueblichen
   Operatorrangfolge. Darum muss ein zusammengesetzter rechter Operand
   geklammert werden:

     (+ 2 (* 3 4))  ->  2 + ( 3 * 4 )

   Ohne Klammern wuerde "3 * 4 =" bereits die noch offene Addition veraendern.
   Ein binaerer Wurzelausdruck endet dagegen mit "=". Innerhalb eines rechten
   Operanden beendet ")" die Rechnung; ein zusaetzliches "=" wuerde die frueher
   beobachtete nutzlose Folge "= =" erzeugen. */
static bool compile_node(const Tree *tree, int idx, uint16_t *codes, int *len,
                         int max_len, int start_address, bool right_operand) {
  if (idx < 0 || idx >= tree->node_count) return false;
  const TreeNode *node = &tree->nodes[idx];
  if (right_operand && node->kind != TN_BINARY &&
      node_contains_binary(tree, idx)) {
    return emit_key(codes, len, max_len, KEY_LPAREN) &&
           compile_node(tree, idx, codes, len, max_len, start_address, false) &&
           emit_key(codes, len, max_len, KEY_RPAREN);
  }
  switch (node->kind) {
    case TN_X:
      /* Der Eingangswert steht bereits im Display. Keine Taste erforderlich. */
      return *len == 0;
    case TN_CONST:
      return emit_key(codes, len, max_len, KEY_DIGIT0 + (node->value % 10));
    case TN_RCL:
      return emit_key(codes, len, max_len, KEY_RCL) &&
             emit_key(codes, len, max_len, KEY_DIGIT0 + (node->value % MEMORY_SIZE));
    case TN_UNARY:
      return compile_node(tree, node->child[0], codes, len, max_len,
                          start_address, false) &&
             emit_key(codes, len, max_len, node->value);
    case TN_NONZERO:
      /* z^2 ist nur fuer z != 0 positiv. Nach +/- ist das Ergebnis dann
         negativ; fuer z == 0 bleibt es 0 und SKP springt nicht. */
      return compile_node(tree, node->child[0], codes, len, max_len,
                          start_address, false) &&
             emit_key(codes, len, max_len, KEY_SQR) &&
             emit_key(codes, len, max_len, KEY_SIGN);
    case TN_BINARY: {
      /* Ein direkt rechts stehender binaerer Ausdruck bekommt genau ein Paar
         Klammern und wird durch ")" statt durch "=" abgeschlossen. */
      bool needs_parens = right_operand;
      if (needs_parens && !emit_key(codes, len, max_len, KEY_LPAREN)) return false;
      if (!compile_node(tree, node->child[0], codes, len, max_len,
                        start_address, false) ||
          !emit_key(codes, len, max_len, node->value) ||
          !compile_node(tree, node->child[1], codes, len, max_len,
                        start_address, true)) {
        return false;
      }
      return emit_key(codes, len, max_len,
                      needs_parens ? KEY_RPAREN : KEY_EQ);
    }
    case TN_MIN:
      /* min(x,y):

           x - y = SKP C/CE + y =

         Bei x < y ist x-y negativ, SKP ueberspringt C/CE und
         (x-y)+y ergibt x. Andernfalls loescht C/CE das Zwischenergebnis,
         danach ergibt 0+y den Wert y. */
      return compile_node(tree, node->child[0], codes, len, max_len,
                          start_address, false) &&
             emit_key(codes, len, max_len, KEY_SUB) &&
             compile_node(tree, node->child[1], codes, len, max_len,
                          start_address, true) &&
             emit_key(codes, len, max_len, KEY_EQ) &&
             emit_key(codes, len, max_len, KEY_SKP) &&
             emit_key(codes, len, max_len, KEY_CLEAR) &&
             emit_key(codes, len, max_len, KEY_ADD) &&
             compile_node(tree, node->child[1], codes, len, max_len,
                          start_address, true) &&
             emit_key(codes, len, max_len, KEY_EQ);
    case TN_MEMOP: {
      int op = node->value / 10;
      int mem = node->value % 10;
      /* Beispiel: (M+ M7 expr) wird zu "expr M+ 7". */
      return compile_node(tree, node->child[0], codes, len, max_len,
                          start_address, false) &&
             emit_key(codes, len, max_len, op) &&
             emit_key(codes, len, max_len, KEY_DIGIT0 + mem);
    }
    case TN_IF: {
      /* SKP ueberspringt bei wahr (X < 0) das folgende dreizellige GOTO.

           condition
           SKP
           GOTO else_or_end
           then
           GOTO end          nur mit Else-Zweig
         else:
           else
         end:

         Die absoluten Ziele werden eingetragen, sobald ihre Zellposition
         bekannt ist. start_address ist die Laenge des Target-Prefix. */
      if (!compile_node(tree, node->child[0], codes, len, max_len,
                        start_address, false) ||
          !emit_key(codes, len, max_len, KEY_SKP) ||
          !emit_key(codes, len, max_len, KEY_GOTO)) {
        return false;
      }
      int false_address_pos = *len;
      if (!emit_key(codes, len, max_len, KEY_DIGIT0) ||
          !emit_key(codes, len, max_len, KEY_DIGIT0) ||
          !compile_node(tree, node->child[1], codes, len, max_len,
                        start_address, false)) {
        return false;
      }

      if (node->child_count == 2) {
        return patch_address(codes, false_address_pos,
                             start_address + *len);
      }

      if (!emit_key(codes, len, max_len, KEY_GOTO)) return false;
      int end_address_pos = *len;
      if (!emit_key(codes, len, max_len, KEY_DIGIT0) ||
          !emit_key(codes, len, max_len, KEY_DIGIT0)) {
        return false;
      }
      if (!patch_address(codes, false_address_pos, start_address + *len) ||
          !compile_node(tree, node->child[2], codes, len, max_len,
                        start_address, false)) {
        return false;
      }
      return patch_address(codes, end_address_pos, start_address + *len);
    }
    case TN_SEQ:
      for (int i = 0; i < node->child_count; i++) {
        if (!compile_node(tree, node->child[i], codes, len, max_len,
                          start_address, false)) {
          return false;
        }
      }
      return true;
  }
  return false;
}

static bool compile_tree(const Tree *tree, uint16_t *codes, int *len,
                         int max_len, int start_address) {
  *len = 0;
  return tree_is_valid(tree) &&
         compile_node(tree, tree->root, codes, len, max_len,
                      start_address, false) &&
         start_address + *len < PROGRAM_SIZE;
}

static int add_binary_node(Tree *tree, int key, int left, int right) {
  int idx = tree_add(tree, TN_BINARY, key);
  if (idx < 0) return -1;
  tree->nodes[idx].child[0] = left;
  tree->nodes[idx].child[1] = right;
  tree->nodes[idx].child_count = 2;
  return idx;
}

static int add_nonzero_node(Tree *tree, int expression) {
  int idx = tree_add(tree, TN_NONZERO, 0);
  if (idx < 0) return -1;
  tree->nodes[idx].child[0] = expression;
  tree->nodes[idx].child_count = 1;
  return idx;
}

static int add_min_node(Tree *tree, int left, int right) {
  int idx = tree_add(tree, TN_MIN, 0);
  if (idx < 0) return -1;
  tree->nodes[idx].child[0] = left;
  tree->nodes[idx].child[1] = right;
  tree->nodes[idx].child_count = 2;
  return idx;
}

/* Kleiner Regressionstest fuer TN_X. Er prueft sowohl die erzeugten Tasten
   als auch die beiden verbotenen Verwendungen des linearen Eingangswerts. */
static bool run_tree_self_test(void) {
  uint16_t codes[MAX_CELLS];
  int len = 0;

  Tree x_only = {0};
  x_only.root = tree_add(&x_only, TN_X, 0);
  if (!compile_tree(&x_only, codes, &len, MAX_CELLS, 0) || len != 0) return false;

  Tree subtract = {0};
  int x = tree_add(&subtract, TN_X, 0);
  int m7 = tree_add(&subtract, TN_RCL, 7);
  subtract.root = add_binary_node(&subtract, KEY_SUB, x, m7);
  if (!compile_tree(&subtract, codes, &len, MAX_CELLS, 0) || len != 4 ||
      codes[0] != KEY_SUB || codes[1] != KEY_RCL ||
      codes[2] != KEY_DIGIT0 + 7 || codes[3] != KEY_EQ) {
    return false;
  }

  Tree late_x = {0};
  m7 = tree_add(&late_x, TN_RCL, 7);
  x = tree_add(&late_x, TN_X, 0);
  late_x.root = add_binary_node(&late_x, KEY_ADD, m7, x);
  if (compile_tree(&late_x, codes, &len, MAX_CELLS, 0)) return false;

  Tree double_x = {0};
  int first_x = tree_add(&double_x, TN_X, 0);
  int second_x = tree_add(&double_x, TN_X, 0);
  double_x.root = add_binary_node(&double_x, KEY_ADD, first_x, second_x);
  if (compile_tree(&double_x, codes, &len, MAX_CELLS, 0)) return false;

  /* Ein IF ist ein Statement und darf nicht als Zahlenoperand erscheinen. */
  Tree typed_if = {0};
  int typed_condition_value = tree_add(&typed_if, TN_CONST, 1);
  int typed_condition = add_nonzero_node(&typed_if, typed_condition_value);
  int typed_value = tree_add(&typed_if, TN_CONST, 1);
  int typed_memop = tree_add(&typed_if, TN_MEMOP, KEY_MPLUS * 10 + 1);
  typed_if.nodes[typed_memop].child[0] = typed_value;
  typed_if.nodes[typed_memop].child_count = 1;
  int typed_statement = tree_add(&typed_if, TN_IF, 0);
  typed_if.nodes[typed_statement].child[0] = typed_condition;
  typed_if.nodes[typed_statement].child[1] = typed_memop;
  typed_if.nodes[typed_statement].child_count = 2;
  int typed_left = tree_add(&typed_if, TN_CONST, 2);
  typed_if.root = add_binary_node(&typed_if, KEY_MUL,
                                  typed_left, typed_statement);
  if (compile_tree(&typed_if, codes, &len, MAX_CELLS, 0)) return false;

  Tree if_tree = {0};
  int condition_x = tree_add(&if_tree, TN_X, 0);
  int condition = add_nonzero_node(&if_tree, condition_x);
  int one = tree_add(&if_tree, TN_CONST, 1);
  int then_memop = tree_add(&if_tree, TN_MEMOP, KEY_MPLUS * 10 + 1);
  if_tree.nodes[then_memop].child[0] = one;
  if_tree.nodes[then_memop].child_count = 1;
  if_tree.root = tree_add(&if_tree, TN_IF, 0);
  if_tree.nodes[if_tree.root].child[0] = condition;
  if_tree.nodes[if_tree.root].child[1] = then_memop;
  if_tree.nodes[if_tree.root].child_count = 2;
  if (!compile_tree(&if_tree, codes, &len, MAX_CELLS, 20) || len != 9 ||
      codes[0] != KEY_SQR || codes[1] != KEY_SIGN ||
      codes[2] != KEY_SKP || codes[3] != KEY_GOTO ||
      codes[4] != KEY_DIGIT0 + 2 || codes[5] != KEY_DIGIT0 + 9 ||
      codes[6] != KEY_DIGIT0 + 1 || codes[7] != KEY_MPLUS ||
      codes[8] != KEY_DIGIT0 + 1) {
    return false;
  }

  /* Fuehrt denselben IF-Code mit 0 sowie mit positiven und negativen
     Nichtnullwerten aus. Nur bei 0 darf M1 unveraendert bleiben. */
  static const double inputs[] = {0.0, 2.0, -3.0};
  for (int test = 0; test < 3; test++) {
    SantronState state;
    init_state(&state);
    for (int i = 0; i < len; i++) state.program[20 + i] = codes[i];
    state.program[20 + len] = KEY_RS;
    state.pc = 20;
    set_x(&state, inputs[test]);
    run_program(&state, 100);
    double expected = inputs[test] != 0.0 ? 1.0 : 0.0;
    if (!nearly_equal(state.memory[1], expected) || !state.paused) return false;
  }

  Tree min_tree = {0};
  int m0 = tree_add(&min_tree, TN_RCL, 0);
  int m1 = tree_add(&min_tree, TN_RCL, 1);
  min_tree.root = add_min_node(&min_tree, m0, m1);
  if (!compile_tree(&min_tree, codes, &len, MAX_CELLS, 0) || len != 12) {
    return false;
  }
  static const double min_inputs[][2] = {
    {2.0, 5.0},
    {8.0, 3.0},
    {4.0, 4.0}
  };
  for (int test = 0; test < 3; test++) {
    SantronState state;
    init_state(&state);
    for (int i = 0; i < len; i++) state.program[i] = codes[i];
    state.program[len] = KEY_RS;
    state.memory[0] = min_inputs[test][0];
    state.memory[1] = min_inputs[test][1];
    run_program(&state, 100);
    double expected = fmin(min_inputs[test][0], min_inputs[test][1]);
    if (!nearly_equal(state.x, expected) || !state.paused) return false;
  }

  Tree parsed = {0};
  if (!parse_ast_line(
        "(seq (M- M9 1) (if (!=0 M7) (M+ M1 1)) (min M2 4))",
        &parsed) ||
      !tree_is_valid(&parsed)) {
    return false;
  }

  return true;
}

static bool score_is_better(const Score *a, const Score *b) {
  if (a->cost != b->cost) return a->cost < b->cost;
  if (a->exact != b->exact) return a->exact > b->exact;
  if (a->matched_checks != b->matched_checks) return a->matched_checks > b->matched_checks;
  /* Die Laenge ist fuer fehlerhafte Programme bedeutungslos. Erst wenn
     beide Kandidaten vollstaendig korrekt sind, suchen wir den kuerzeren. */
  bool a_complete = a->exact == a->total &&
                    a->matched_checks == a->total_checks &&
                    a->penalty == 0.0;
  bool b_complete = b->exact == b->total &&
                    b->matched_checks == b->total_checks &&
                    b->penalty == 0.0;
  return a_complete && b_complete && a->cells < b->cells;
}

static void evaluate_candidate(Candidate *candidate, const Target *target, int max_cells) {
  bool ok = compile_tree(&candidate->tree, candidate->codes,
                         &candidate->code_len, max_cells,
                         target->prefix_len);
  if (!ok) {
    memset(&candidate->score, 0, sizeof(candidate->score));
    candidate->score.cost = INFINITY;
    candidate->score.cells = max_cells + 1;
    candidate->evaluation_count = target_evaluation_count(target);
    for (int i = 0; i < candidate->evaluation_count; i++) {
      candidate->evaluation_loss[i] = INFINITY;
    }
    return;
  }
  candidate->score =
    score_codes(candidate->codes, candidate->code_len, target,
                candidate->evaluation_loss, &candidate->evaluation_count);
}

/* Lexicase ordnet die Testfaelle fuer jede Elternwahl neu. Nach jedem Fall
   bleiben nur Kandidaten mit dem aktuell besten Einzelverlust uebrig. So
   koennen Programme ueberleben, die nur fuer einen Teil der Aufgabe bereits
   gut sind, statt von der aggregierten Mehrheit sofort verdraengt zu werden. */
static int select_lexicase_parent(const Candidate *population,
                                  int population_size, int evaluation_count) {
  int survivors[10000];
  int order[MAX_EVALUATIONS];
  for (int i = 0; i < population_size; i++) survivors[i] = i;
  for (int i = 0; i < evaluation_count; i++) order[i] = i;
  for (int i = evaluation_count - 1; i > 0; i--) {
    int j = rng_int(i + 1);
    int tmp = order[i];
    order[i] = order[j];
    order[j] = tmp;
  }

  int survivor_count = population_size;
  for (int position = 0;
       position < evaluation_count && survivor_count > 1;
       position++) {
    int evaluation = order[position];
    double best = INFINITY;
    for (int i = 0; i < survivor_count; i++) {
      double loss = population[survivors[i]].evaluation_loss[evaluation];
      if (loss < best) best = loss;
    }

    /* Numerische Rundung soll nicht zwischen praktisch gleichen Programmen
       entscheiden. Die Toleranz skaliert mit dem besten Verlust. */
    double epsilon = 1e-9 * fmax(1.0, fabs(best));
    int kept = 0;
    for (int i = 0; i < survivor_count; i++) {
      double loss = population[survivors[i]].evaluation_loss[evaluation];
      if (loss <= best + epsilon) survivors[kept++] = survivors[i];
    }
    survivor_count = kept;
  }
  return survivors[rng_int(survivor_count)];
}

static int compare_candidates(const void *left, const void *right) {
  const Candidate *a = (const Candidate *)left;
  const Candidate *b = (const Candidate *)right;
  if (score_is_better(&a->score, &b->score)) return -1;
  if (score_is_better(&b->score, &a->score)) return 1;
  return 0;
}

static void print_codes(const uint16_t *codes, int len) {
  for (int i = 0; i < len; i++) printf("%s%s", i ? " " : "", code_name(codes[i]));
  printf("\n");
}

static void write_tree_node(FILE *out, const Tree *tree, int idx) {
  const TreeNode *node = &tree->nodes[idx];
  switch (node->kind) {
    case TN_X:
      fprintf(out, "X");
      break;
    case TN_CONST:
      fprintf(out, "%d", node->value % 10);
      break;
    case TN_RCL:
      fprintf(out, "M%d", node->value % MEMORY_SIZE);
      break;
    case TN_UNARY:
      fprintf(out, "(%s ", code_name(node->value));
      write_tree_node(out, tree, node->child[0]);
      fprintf(out, ")");
      break;
    case TN_NONZERO:
      fprintf(out, "(!=0 ");
      write_tree_node(out, tree, node->child[0]);
      fprintf(out, ")");
      break;
    case TN_BINARY:
      fprintf(out, "(%s ", code_name(node->value));
      write_tree_node(out, tree, node->child[0]);
      fprintf(out, " ");
      write_tree_node(out, tree, node->child[1]);
      fprintf(out, ")");
      break;
    case TN_MIN:
      fprintf(out, "(min ");
      write_tree_node(out, tree, node->child[0]);
      fprintf(out, " ");
      write_tree_node(out, tree, node->child[1]);
      fprintf(out, ")");
      break;
    case TN_MEMOP:
      fprintf(out, "(%s M%d ", code_name(node->value / 10), node->value % 10);
      write_tree_node(out, tree, node->child[0]);
      fprintf(out, ")");
      break;
    case TN_IF:
      fprintf(out, "(if ");
      write_tree_node(out, tree, node->child[0]);
      fprintf(out, " ");
      write_tree_node(out, tree, node->child[1]);
      if (node->child_count == 3) {
        fprintf(out, " ");
        write_tree_node(out, tree, node->child[2]);
      }
      fprintf(out, ")");
      break;
    case TN_SEQ:
      fprintf(out, "(seq");
      for (int i = 0; i < node->child_count; i++) {
        fprintf(out, " ");
        write_tree_node(out, tree, node->child[i]);
      }
      fprintf(out, ")");
      break;
  }
}

static void print_candidate(const Candidate *candidate) {
  printf("cells=%d nodes=%d exact=%d/%d checks=%d/%d error=%.12g penalty=%.12g cost=%.12g\n",
         candidate->code_len, candidate->tree.node_count,
         candidate->score.exact, candidate->score.total,
         candidate->score.matched_checks, candidate->score.total_checks,
         candidate->score.error, candidate->score.penalty, candidate->score.cost);
  print_codes(candidate->codes, candidate->code_len);
  write_tree_node(stdout, &candidate->tree, candidate->tree.root);
  printf("\n");
}

typedef struct {
  const char *cursor;
  bool error;
} AstParser;

static void ast_skip_space(AstParser *parser) {
  while (isspace((unsigned char)*parser->cursor)) parser->cursor++;
}

static bool ast_read_token(AstParser *parser, char *token, size_t token_size) {
  ast_skip_space(parser);
  const char *start = parser->cursor;
  while (*parser->cursor &&
         !isspace((unsigned char)*parser->cursor) &&
         *parser->cursor != '(' && *parser->cursor != ')') {
    parser->cursor++;
  }
  size_t length = (size_t)(parser->cursor - start);
  if (length == 0 || length >= token_size) {
    parser->error = true;
    return false;
  }
  memcpy(token, start, length);
  token[length] = '\0';
  return true;
}

static bool ast_consume(AstParser *parser, char expected) {
  ast_skip_space(parser);
  if (*parser->cursor != expected) {
    parser->error = true;
    return false;
  }
  parser->cursor++;
  return true;
}

static bool unary_ast_key(const char *token, int *key) {
  if (!parse_key_token(token, key)) return false;
  return *key == KEY_SIGN || *key == KEY_INV || *key == KEY_SQR ||
         *key == KEY_SQRT || *key == KEY_LN || *key == KEY_LOG ||
         *key == KEY_EX || *key == KEY_TENX || *key == KEY_SIN ||
         *key == KEY_COS || *key == KEY_TAN || *key == KEY_ASIN ||
         *key == KEY_ACOS || *key == KEY_ATAN;
}

static bool binary_ast_key(const char *token, int *key) {
  if (!parse_key_token(token, key)) return false;
  return *key == KEY_ADD || *key == KEY_SUB || *key == KEY_MUL ||
         *key == KEY_DIV || *key == KEY_YX;
}

static bool memop_ast_key(const char *token, int *key) {
  if (!parse_key_token(token, key)) return false;
  return *key == KEY_STO || *key == KEY_MPLUS || *key == KEY_MMINUS ||
         *key == KEY_MULMEM || *key == KEY_DIVMEM;
}

static int parse_ast_node(AstParser *parser, Tree *tree);

static int parse_ast_list(AstParser *parser, Tree *tree) {
  char operator_name[32];
  if (!ast_read_token(parser, operator_name, sizeof(operator_name))) return -1;

  if (strcmp(operator_name, "seq") == 0) {
    int idx = tree_add(tree, TN_SEQ, 0);
    if (idx < 0) {
      parser->error = true;
      return -1;
    }
    while (true) {
      ast_skip_space(parser);
      if (*parser->cursor == ')') break;
      if (tree->nodes[idx].child_count >= MAX_SEQ_CHILDREN) {
        parser->error = true;
        return -1;
      }
      int child = parse_ast_node(parser, tree);
      if (child < 0) return -1;
      tree->nodes[idx].child[tree->nodes[idx].child_count++] = child;
    }
    return ast_consume(parser, ')') ? idx : -1;
  }

  if (strcmp(operator_name, "if") == 0) {
    int idx = tree_add(tree, TN_IF, 0);
    if (idx < 0) {
      parser->error = true;
      return -1;
    }
    for (int i = 0; i < 2; i++) {
      int child = parse_ast_node(parser, tree);
      if (child < 0) return -1;
      tree->nodes[idx].child[tree->nodes[idx].child_count++] = child;
    }
    ast_skip_space(parser);
    if (*parser->cursor != ')') {
      int child = parse_ast_node(parser, tree);
      if (child < 0) return -1;
      tree->nodes[idx].child[tree->nodes[idx].child_count++] = child;
    }
    return ast_consume(parser, ')') ? idx : -1;
  }

  if (strcmp(operator_name, "!=0") == 0) {
    int idx = tree_add(tree, TN_NONZERO, 0);
    if (idx < 0) {
      parser->error = true;
      return -1;
    }
    tree->nodes[idx].child[0] = parse_ast_node(parser, tree);
    tree->nodes[idx].child_count = 1;
    if (tree->nodes[idx].child[0] < 0) return -1;
    return ast_consume(parser, ')') ? idx : -1;
  }

  if (strcmp(operator_name, "min") == 0) {
    int idx = tree_add(tree, TN_MIN, 0);
    if (idx < 0) {
      parser->error = true;
      return -1;
    }
    tree->nodes[idx].child[0] = parse_ast_node(parser, tree);
    tree->nodes[idx].child[1] = parse_ast_node(parser, tree);
    tree->nodes[idx].child_count = 2;
    if (tree->nodes[idx].child[0] < 0 || tree->nodes[idx].child[1] < 0) return -1;
    return ast_consume(parser, ')') ? idx : -1;
  }

  int key = 0;
  if (memop_ast_key(operator_name, &key)) {
    char memory_name[16];
    if (!ast_read_token(parser, memory_name, sizeof(memory_name)) ||
        memory_name[0] != 'M' || !isdigit((unsigned char)memory_name[1]) ||
        memory_name[2] != '\0') {
      parser->error = true;
      return -1;
    }
    int idx = tree_add(tree, TN_MEMOP,
                       key * 10 + memory_name[1] - '0');
    if (idx < 0) {
      parser->error = true;
      return -1;
    }
    tree->nodes[idx].child[0] = parse_ast_node(parser, tree);
    tree->nodes[idx].child_count = 1;
    if (tree->nodes[idx].child[0] < 0) return -1;
    return ast_consume(parser, ')') ? idx : -1;
  }

  if (unary_ast_key(operator_name, &key)) {
    int idx = tree_add(tree, TN_UNARY, key);
    if (idx < 0) {
      parser->error = true;
      return -1;
    }
    tree->nodes[idx].child[0] = parse_ast_node(parser, tree);
    tree->nodes[idx].child_count = 1;
    if (tree->nodes[idx].child[0] < 0) return -1;
    return ast_consume(parser, ')') ? idx : -1;
  }

  if (binary_ast_key(operator_name, &key)) {
    int idx = tree_add(tree, TN_BINARY, key);
    if (idx < 0) {
      parser->error = true;
      return -1;
    }
    tree->nodes[idx].child[0] = parse_ast_node(parser, tree);
    tree->nodes[idx].child[1] = parse_ast_node(parser, tree);
    tree->nodes[idx].child_count = 2;
    if (tree->nodes[idx].child[0] < 0 || tree->nodes[idx].child[1] < 0) return -1;
    return ast_consume(parser, ')') ? idx : -1;
  }

  parser->error = true;
  return -1;
}

static int parse_ast_node(AstParser *parser, Tree *tree) {
  ast_skip_space(parser);
  if (*parser->cursor == '(') {
    parser->cursor++;
    return parse_ast_list(parser, tree);
  }

  char atom[16];
  if (!ast_read_token(parser, atom, sizeof(atom))) return -1;
  if (strcmp(atom, "X") == 0) return tree_add(tree, TN_X, 0);
  if (atom[0] == 'M' && isdigit((unsigned char)atom[1]) && atom[2] == '\0') {
    return tree_add(tree, TN_RCL, atom[1] - '0');
  }
  if (isdigit((unsigned char)atom[0]) && atom[1] == '\0') {
    return tree_add(tree, TN_CONST, atom[0] - '0');
  }
  parser->error = true;
  return -1;
}

static bool parse_ast_line(const char *line, Tree *tree) {
  memset(tree, 0, sizeof(*tree));
  AstParser parser = {.cursor = line, .error = false};
  tree->root = parse_ast_node(&parser, tree);
  ast_skip_space(&parser);
  return !parser.error && tree->root >= 0 && *parser.cursor == '\0' &&
         tree_is_valid(tree);
}

static bool same_program(const Candidate *a, const Candidate *b) {
  return a->code_len == b->code_len &&
         memcmp(a->codes, b->codes,
                (size_t)a->code_len * sizeof(a->codes[0])) == 0;
}

static bool top_file_path(char *path, size_t path_size,
                          const char *prefix, int index) {
  int written = snprintf(path, path_size, "%s-%d.ast", prefix, index);
  return written > 0 && (size_t)written < path_size;
}

/* Schreibt eine komplette Generation direkt in eine der rotierenden Dateien.
   Wird der Prozess dabei beendet, ist hoechstens diese eine Datei
   unvollstaendig; die anderen Generationen bleiben unveraendert. */
static bool write_top_programs(const char *prefix, int file_index,
                               int generation, const Candidate *population,
                               int population_size, int top_count) {
  char path[1024];
  if (!top_file_path(path, sizeof(path), prefix, file_index)) return false;
  FILE *file = fopen(path, "w");
  if (!file) {
    fprintf(stderr, "warning: cannot write %s\n", path);
    return false;
  }

  fprintf(file, "# santron-tree-search AST snapshot\n");
  fprintf(file, "# generation=%d requested=%d\n", generation, top_count);
  int written = 0;
  for (int i = 0; i < population_size && written < top_count; i++) {
    bool duplicate = false;
    for (int previous = 0; previous < i; previous++) {
      if (same_program(&population[i], &population[previous])) {
        duplicate = true;
        break;
      }
    }
    if (duplicate || !isfinite(population[i].score.cost)) continue;
    write_tree_node(file, &population[i].tree, population[i].tree.root);
    fputc('\n', file);
    written++;
  }
  fprintf(file, "# complete generation=%d count=%d\n", generation, written);
  bool ok = fflush(file) == 0 && !ferror(file);
  if (fclose(file) != 0) ok = false;
  if (!ok) fprintf(stderr, "warning: incomplete write of %s\n", path);
  return ok;
}

/* Laedt alle einzeln parsebaren Zeilen. Kommentare, abgeschnittene Zeilen,
   ungueltige ASTs und Duplikate werden ignoriert. Jeder AST wird mit dem
   aktuellen Target und Scorer neu bewertet. */
static int load_top_programs(const char *prefix, int file_count,
                             Candidate *population, int population_size,
                             const Target *target, int max_cells) {
  int loaded = 0;
  char path[1024];
  char line[MAX_AST_LINE];
  for (int file_index = 0; file_index < file_count; file_index++) {
    if (!top_file_path(path, sizeof(path), prefix, file_index)) continue;
    FILE *file = fopen(path, "r");
    if (!file) continue;
    int accepted_from_file = 0;
    int rejected_from_file = 0;
    while (loaded < population_size && fgets(line, sizeof(line), file)) {
      size_t length = strlen(line);
      bool complete_line = length > 0 && line[length - 1] == '\n';
      if (!complete_line && !feof(file)) {
        int ch;
        while ((ch = fgetc(file)) != '\n' && ch != EOF) {}
        rejected_from_file++;
        continue;
      }
      char *text = trim(line);
      if (*text == '\0' || *text == '#') continue;

      Candidate candidate;
      memset(&candidate, 0, sizeof(candidate));
      if (!parse_ast_line(text, &candidate.tree)) {
        rejected_from_file++;
        continue;
      }
      evaluate_candidate(&candidate, target, max_cells);
      if (!isfinite(candidate.score.cost)) {
        rejected_from_file++;
        continue;
      }
      bool duplicate = false;
      for (int i = 0; i < loaded; i++) {
        if (same_program(&candidate, &population[i])) {
          duplicate = true;
          break;
        }
      }
      if (duplicate) continue;
      population[loaded++] = candidate;
      accepted_from_file++;
    }
    fclose(file);
    printf("top-load file=%s accepted=%d rejected=%d\n",
           path, accepted_from_file, rejected_from_file);
  }
  return loaded;
}

static uint64_t current_unix_millis(void) {
#if defined(_POSIX_TIMERS) && _POSIX_TIMERS > 0
  struct timespec ts;
  if (clock_gettime(CLOCK_REALTIME, &ts) == 0) {
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

static int parse_int_arg(int argc, char **argv, const char *name, int default_value) {
  const char *text = parse_string_arg(argc, argv, name, NULL);
  if (!text) return default_value;
  return atoi(text);
}

static uint64_t parse_u64_arg(int argc, char **argv, const char *name, uint64_t default_value) {
  const char *text = parse_string_arg(argc, argv, name, NULL);
  if (!text) return default_value;
  char *end = NULL;
  unsigned long long value = strtoull(text, &end, 10);
  return end != text && *end == '\0' ? (uint64_t)value : default_value;
}

static bool has_flag(int argc, char **argv, const char *name) {
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], name) == 0) return true;
  }
  return false;
}

static void usage(const char *argv0) {
  fprintf(stderr,
          "usage: %s --target file [--population n] [--generations n] [--depth n] [--elite n] [--immigrants n] [--point-mutation n] [--subtree-mutation n] [--selection lexicase|aggregate] [--max-cells n] [--top-prefix path] [--top-files n] [--top-count n] [--seed n|--seed-ms]\n"
          "       %s --target file --candidate \"Santron keys\"\n"
          "       %s --self-test\n",
          argv0,
          argv0,
          argv0);
}

int main(int argc, char **argv) {
  if (has_flag(argc, argv, "--self-test")) {
    bool ok = run_tree_self_test();
    printf("tree self-test: %s\n", ok ? "PASS" : "FAIL");
    return ok ? 0 : 1;
  }

  const char *target_path = parse_string_arg(argc, argv, "--target", NULL);
  if (!target_path) {
    usage(argv[0]);
    return 2;
  }

  Target target;
  parse_target(target_path, &target);

  int population_size = parse_int_arg(argc, argv, "--population", 200);
  int generations = parse_int_arg(argc, argv, "--generations", 100);
  int depth = parse_int_arg(argc, argv, "--depth", 4);
  int elite = parse_int_arg(argc, argv, "--elite", population_size / 10);
  int immigrants = parse_int_arg(argc, argv, "--immigrants",
                                 population_size / 20);
  int point_mutation = parse_int_arg(argc, argv, "--point-mutation", 50);
  int subtree_mutation = parse_int_arg(argc, argv, "--subtree-mutation", 30);
  const char *selection =
    parse_string_arg(argc, argv, "--selection", "lexicase");
  bool use_lexicase = strcmp(selection, "lexicase") == 0;
  bool use_aggregate = strcmp(selection, "aggregate") == 0;
  int max_cells = parse_int_arg(argc, argv, "--max-cells", target.max_cells);
  const char *top_prefix = parse_string_arg(argc, argv, "--top-prefix", NULL);
  int top_files = parse_int_arg(argc, argv, "--top-files", 3);
  int top_count = parse_int_arg(argc, argv, "--top-count", 100);
  int trailing_cells = target.append_len > 0 ? target.append_len : 1;
  int available_cells = PROGRAM_SIZE - target.prefix_len - trailing_cells;
  int objective_count = target_evaluation_count(&target);
  uint64_t seed = has_flag(argc, argv, "--seed-ms")
                    ? current_unix_millis()
                    : parse_u64_arg(argc, argv, "--seed", current_unix_millis());

  if (population_size <= 0 || population_size > 10000 || generations < 0 ||
      depth <= 0 || elite <= 0 || elite > population_size ||
      immigrants < 0 || elite + immigrants > population_size ||
      point_mutation < 0 || subtree_mutation < 0 ||
      point_mutation + subtree_mutation > 100 ||
      top_files <= 0 || top_files > MAX_TOP_FILES ||
      top_count <= 0 || top_count > population_size ||
      (!use_lexicase && !use_aggregate) ||
      objective_count <= 0 || objective_count > MAX_EVALUATIONS ||
      available_cells <= 0 || max_cells <= 0 || max_cells > available_cells) {
    fprintf(stderr,
            "invalid parameters: --max-cells must be 1..%d and target "
            "objectives must be 1..%d (got %d)\n",
            available_cells, MAX_EVALUATIONS, objective_count);
    return 2;
  }

  const char *candidate_text =
    parse_string_arg(argc, argv, "--candidate", NULL);
  if (candidate_text) {
    char text[MAX_LINE];
    if (strlen(candidate_text) >= sizeof(text)) {
      fprintf(stderr, "candidate is too long\n");
      return 2;
    }
    snprintf(text, sizeof(text), "%s", candidate_text);
    uint16_t codes[MAX_CELLS];
    int code_len = parse_key_sequence(text, codes, MAX_CELLS,
                                      "<candidate>", 1);
    if (code_len > max_cells) {
      fprintf(stderr, "candidate has %d cells, limit is %d\n",
              code_len, max_cells);
      return 2;
    }
    float losses[MAX_EVALUATIONS];
    int loss_count = 0;
    Score score = score_codes(codes, code_len, &target,
                              losses, &loss_count);
    printf("candidate cells=%d exact=%d/%d checks=%d/%d error=%.12g "
           "penalty=%.12g cost=%.12g objectives=%d\n",
           score.cells, score.exact, score.total,
           score.matched_checks, score.total_checks,
           score.error, score.penalty, score.cost, loss_count);
    return score.exact == score.total &&
           score.matched_checks == score.total_checks ? 0 : 1;
  }

  rng_state = seed ? seed : 1;
  Candidate *population = calloc((size_t)population_size, sizeof(*population));
  Candidate *next = calloc((size_t)population_size, sizeof(*next));
  if (!population || !next) {
    fprintf(stderr, "out of memory\n");
    free(population);
    free(next);
    return 2;
  }

  printf("target=%s seed=%llu population=%d generations=%d depth=%d elite=%d "
         "immigrants=%d variation=%d%%-point/%d%%-subtree/%d%%-crossover "
         "selection=%s max-cells=%d target-default=%d "
         "available-cells=%d objectives=%d score=%s top=%s/%d/%d\n",
         target.name, (unsigned long long)seed, population_size, generations,
         depth, elite, immigrants, point_mutation, subtree_mutation,
         100 - point_mutation - subtree_mutation, selection,
         max_cells, target.max_cells,
         available_cells, objective_count,
         target.modern_score ? "target" : "legacy",
         top_prefix ? top_prefix : "-", top_files, top_count);

  int loaded = top_prefix
                 ? load_top_programs(top_prefix, top_files, population,
                                     population_size, &target, max_cells)
                 : 0;
  printf("initial seeds loaded=%d random=%d\n",
         loaded, population_size - loaded);
  for (int i = loaded; i < population_size; i++) {
    population[i].tree = random_tree(depth);
    evaluate_candidate(&population[i], &target, max_cells);
  }

  Candidate best = population[0];
  for (int generation = 0; generation <= generations; generation++) {
    qsort(population, (size_t)population_size, sizeof(population[0]), compare_candidates);
    if (generation == 0 || score_is_better(&population[0].score, &best.score)) {
      best = population[0];
      printf("\nnew best generation=%d\n", generation);
      print_candidate(&best);
      fflush(stdout);
    }
    if (top_prefix) {
      int file_index = generation % top_files;
      if (write_top_programs(top_prefix, file_index, generation,
                             population, population_size, top_count)) {
        printf("top-save generation=%d file=%s-%d.ast count<=%d\n",
               generation, top_prefix, file_index, top_count);
        fflush(stdout);
      }
    }
    if (generation == generations) break;

    for (int i = 0; i < elite; i++) next[i] = population[i];
    int offspring_end = population_size - immigrants;
    for (int i = elite; i < offspring_end; i++) {
      int a = use_lexicase
                ? select_lexicase_parent(population, population_size,
                                         population[0].evaluation_count)
                : rng_int(elite);
      int b = use_lexicase
                ? select_lexicase_parent(population, population_size,
                                         population[0].evaluation_count)
                : rng_int(elite);
      int variation = rng_int(100);
      if (variation < point_mutation) {
        next[i].tree = point_mutate_tree(&population[a].tree);
      } else if (variation < point_mutation + subtree_mutation) {
        next[i].tree = mutate_tree(&population[a].tree, depth);
      } else {
        next[i].tree = crossover_tree(&population[a].tree, &population[b].tree);
      }
      evaluate_candidate(&next[i], &target, max_cells);
    }
    for (int i = offspring_end; i < population_size; i++) {
      next[i].tree = random_tree(depth);
      evaluate_candidate(&next[i], &target, max_cells);
    }
    Candidate *tmp = population;
    population = next;
    next = tmp;
  }

  printf("\noverall best:\n");
  print_candidate(&best);

  int result = best.score.exact == best.score.total &&
               best.score.penalty == 0.0 &&
               best.score.matched_checks == best.score.total_checks ? 0 : 1;
  free(population);
  free(next);
  return result;
}
