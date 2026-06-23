#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define PROGRAM_SIZE 100
#define MEMORY_SIZE 10
#define STACK_SIZE 8

enum {
  KEY_ADD = 10,
  KEY_SUB = 11,
  KEY_MUL = 12,
  KEY_DIV = 13,
  KEY_YX = 14,
  KEY_INV = 17,
  KEY_SQR = 18,
  KEY_SQRT = 19,
  KEY_SIN = 20,
  KEY_COS = 21,
  KEY_TAN = 22,
  KEY_ASIN = 30,
  KEY_ACOS = 31,
  KEY_ATAN = 32,
  KEY_LN = 40,
  KEY_LOG = 41,
  KEY_EX = 42,
  KEY_TENX = 43,
  KEY_MPLUS = 50,
  KEY_MMINUS = 51,
  KEY_MULMEM = 52,
  KEY_DIVMEM = 53,
  KEY_STO = 55,
  KEY_RCL = 56,
  KEY_CLEAR = 60,
  KEY_SIGN = 62,
  KEY_EXP = 63,
  KEY_SWAP = 64,
  KEY_PI = 65,
  KEY_DOT = 70,
  KEY_PS = 71,
  KEY_EQ = 80,
  KEY_LPAREN = 81,
  KEY_RPAREN = 82,
  KEY_RS = 90,
  KEY_GOTO = 93,
  KEY_SKP = 96,
  KEY_BLANK = 99,
  KEY_DIGIT0 = 100
};

typedef enum {
  OP_NONE = 0,
  OP_ADD,
  OP_SUB,
  OP_MUL,
  OP_DIV,
  OP_YX
} PendingOp;

typedef struct {
  double y;
  PendingOp pending;
} ParenFrame;

typedef struct {
  double x;
  double y;
  PendingOp pending;
  ParenFrame paren_stack[STACK_SIZE];
  int paren_depth;
  bool entering;
  bool decimal_entry;
  double decimal_divisor;
  bool exponent_entry;
  double exponent_mantissa;
  int exponent_sign;
  int exponent_digits;
  int exponent_digit_count;
  bool pending_precision;
  int pc;
  int program[PROGRAM_SIZE];
  double memory[MEMORY_SIZE];
  bool running;
  bool paused;
  int pending_memory;
} SantronState;

static double round_internal(double value) {
  if (!isfinite(value) || value == 0.0) return value;
  double sign = value < 0.0 ? -1.0 : 1.0;
  double abs_value = fabs(value);
  double exponent = floor(log10(abs_value));
  double scale = pow(10.0, 9.0 - exponent);
  double scaled = abs_value * scale;
  double tolerance = fabs(scaled) * 2.220446049250313e-16 * 8.0;
  return sign * (trunc(scaled + tolerance) / scale);
}

static void init_state(SantronState *s) {
  memset(s, 0, sizeof(*s));
  for (int i = 0; i < PROGRAM_SIZE; i++) s->program[i] = KEY_BLANK;
  s->pending = OP_NONE;
  s->pending_memory = -1;
  s->exponent_sign = 1;
  s->decimal_divisor = 10.0;
}

static int digit_from_code(int code) {
  if (code >= 100 && code <= 109) return code - 100;
  return -1;
}

static bool is_memory_command(int code) {
  return code == KEY_STO || code == KEY_RCL || code == KEY_MPLUS ||
         code == KEY_MMINUS || code == KEY_MULMEM || code == KEY_DIVMEM;
}

static void set_x(SantronState *s, double value) {
  s->x = round_internal(value);
  s->entering = false;
  s->decimal_entry = false;
  s->decimal_divisor = 10.0;
  s->exponent_entry = false;
  s->exponent_digits = 0;
  s->exponent_digit_count = 0;
  s->exponent_sign = 1;
}

static double apply_pending_operator(double left, PendingOp op, double right) {
  switch (op) {
    case OP_ADD: return round_internal(left + right);
    case OP_SUB: return round_internal(left - right);
    case OP_MUL: return round_internal(left * right);
    case OP_DIV: return round_internal(left / right);
    case OP_YX: return round_internal(pow(left, right));
    case OP_NONE: return round_internal(right);
  }
  return round_internal(right);
}

static double commit_pending_operator(SantronState *s) {
  if (s->pending == OP_NONE) return s->x;
  double result = apply_pending_operator(s->y, s->pending, s->x);
  s->y = result;
  set_x(s, result);
  s->pending = OP_NONE;
  return result;
}

static void queue_operator(SantronState *s, PendingOp op) {
  s->exponent_entry = false;
  commit_pending_operator(s);
  s->y = s->x;
  s->pending = op;
  s->entering = false;
}

static void apply_memory_command(SantronState *s, int command, int idx) {
  if (idx < 0 || idx >= MEMORY_SIZE) return;
  double x = round_internal(s->x);
  switch (command) {
    case KEY_STO: s->memory[idx] = x; break;
    case KEY_RCL: set_x(s, s->memory[idx]); break;
    case KEY_MPLUS: s->memory[idx] = round_internal(s->memory[idx] + x); break;
    case KEY_MMINUS: s->memory[idx] = round_internal(s->memory[idx] - x); break;
    case KEY_MULMEM: s->memory[idx] = round_internal(s->memory[idx] * x); break;
    case KEY_DIVMEM: s->memory[idx] = round_internal(s->memory[idx] / x); break;
  }
  s->exponent_entry = false;
  s->entering = false;
  s->decimal_entry = false;
  s->decimal_divisor = 10.0;
}

static void update_exponent_value(SantronState *s) {
  int exponent = s->exponent_sign * s->exponent_digits;
  s->x = round_internal(s->exponent_mantissa * pow(10.0, exponent));
  s->entering = false;
}

static void start_exponent_entry(SantronState *s) {
  s->exponent_mantissa = s->entering ? s->x : 1.0;
  s->x = s->exponent_mantissa;
  s->exponent_entry = true;
  s->exponent_sign = 1;
  s->exponent_digits = 0;
  s->exponent_digit_count = 0;
  s->entering = false;
}

static void push_digit(SantronState *s, int digit) {
  if (s->pending_precision) {
    s->pending_precision = false;
    s->entering = false;
    s->decimal_entry = false;
    s->decimal_divisor = 10.0;
    return;
  }

  if (s->exponent_entry) {
    if (s->exponent_digit_count < 2) {
      s->exponent_digits = s->exponent_digits * 10 + digit;
      s->exponent_digit_count++;
      update_exponent_value(s);
    }
    return;
  }

  if (s->decimal_entry) {
    double sign = s->x < 0.0 ? -1.0 : 1.0;
    s->x += sign * ((double)digit / s->decimal_divisor);
    s->decimal_divisor *= 10.0;
    s->entering = true;
    return;
  }

  if (!s->entering || s->x == 0.0) {
    s->x = (double)digit;
    s->entering = true;
    return;
  }

  double sign = s->x < 0.0 ? -1.0 : 1.0;
  s->x = sign * (fabs(s->x) * 10.0 + digit);
}

static void push_decimal_point(SantronState *s) {
  if (s->exponent_entry) return;
  if (!s->entering) {
    s->x = 0.0;
    s->entering = true;
  }
  s->decimal_entry = true;
  if (s->decimal_divisor <= 1.0) s->decimal_divisor = 10.0;
}

static void clear_expression(SantronState *s) {
  s->x = 0.0;
  s->y = 0.0;
  s->pending = OP_NONE;
  s->paren_depth = 0;
  s->entering = false;
  s->decimal_entry = false;
  s->decimal_divisor = 10.0;
  s->exponent_entry = false;
  s->pending_memory = -1;
}

static void close_paren(SantronState *s) {
  if (s->paren_depth <= 0) return;
  double value = commit_pending_operator(s);
  ParenFrame frame = s->paren_stack[--s->paren_depth];
  s->y = frame.y;
  s->pending = frame.pending;
  set_x(s, value);
}

static void execute_key(SantronState *s, int code, bool from_program);

static void execute_regular_key(SantronState *s, int code, bool from_program) {
  int digit = digit_from_code(code);

  if (s->pending_memory >= 0 && digit >= 0) {
    int command = s->pending_memory;
    s->pending_memory = -1;
    apply_memory_command(s, command, digit);
    return;
  }

  if (digit >= 0) {
    push_digit(s, digit);
    return;
  }

  switch (code) {
    case KEY_ADD: queue_operator(s, OP_ADD); break;
    case KEY_SUB: queue_operator(s, OP_SUB); break;
    case KEY_MUL: queue_operator(s, OP_MUL); break;
    case KEY_DIV: queue_operator(s, OP_DIV); break;
    case KEY_YX: queue_operator(s, OP_YX); break;
    case KEY_EQ:
      while (s->paren_depth > 0) close_paren(s);
      if (s->pending != OP_NONE) commit_pending_operator(s);
      else set_x(s, s->x);
      break;
    case KEY_CLEAR:
      clear_expression(s);
      break;
    case KEY_SIGN:
      if (s->exponent_entry) {
        s->exponent_sign *= -1;
        update_exponent_value(s);
      } else {
        set_x(s, -s->x);
      }
      break;
    case KEY_EXP:
      start_exponent_entry(s);
      break;
    case KEY_SWAP: {
      double old_x = s->x;
      set_x(s, s->y);
      s->y = old_x;
      break;
    }
    case KEY_LPAREN:
      if (s->paren_depth < STACK_SIZE) {
        s->paren_stack[s->paren_depth++] = (ParenFrame){s->y, s->pending};
        s->y = 0.0;
        s->pending = OP_NONE;
        s->x = 0.0;
        s->entering = false;
        s->decimal_entry = false;
        s->decimal_divisor = 10.0;
        s->exponent_entry = false;
      }
      break;
    case KEY_RPAREN:
      close_paren(s);
      break;
    case KEY_TENX:
      set_x(s, pow(10.0, s->x));
      break;
    case KEY_EX:
      set_x(s, exp(s->x));
      break;
    case KEY_LN:
      set_x(s, log(s->x));
      break;
    case KEY_LOG:
      set_x(s, log10(s->x));
      break;
    case KEY_SIN:
      set_x(s, sin(s->x * M_PI / 180.0));
      break;
    case KEY_COS:
      set_x(s, cos(s->x * M_PI / 180.0));
      break;
    case KEY_TAN:
      set_x(s, tan(s->x * M_PI / 180.0));
      break;
    case KEY_ASIN:
      set_x(s, asin(s->x) * 180.0 / M_PI);
      break;
    case KEY_ACOS:
      set_x(s, acos(s->x) * 180.0 / M_PI);
      break;
    case KEY_ATAN:
      set_x(s, atan(s->x) * 180.0 / M_PI);
      break;
    case KEY_SQR:
      set_x(s, s->x * s->x);
      break;
    case KEY_SQRT:
      set_x(s, sqrt(s->x));
      break;
    case KEY_INV:
      set_x(s, 1.0 / s->x);
      break;
    case KEY_PI:
      set_x(s, 3.141592653);
      break;
    case KEY_DOT:
      push_decimal_point(s);
      break;
    case KEY_PS:
      s->pending_precision = true;
      s->entering = false;
      s->decimal_entry = false;
      s->decimal_divisor = 10.0;
      break;
    case KEY_STO:
    case KEY_RCL:
    case KEY_MPLUS:
    case KEY_MMINUS:
    case KEY_MULMEM:
    case KEY_DIVMEM:
      s->pending_memory = code;
      break;
    case KEY_RS:
      if (from_program) {
        s->running = false;
        s->paused = true;
        s->pending_memory = -1;
      } else {
        s->running = true;
        s->paused = false;
      }
      break;
    case KEY_SKP:
      if (from_program) {
        if (s->x < 0.0) {
          int skip_width = s->program[s->pc] == KEY_GOTO ? 3 : 1;
          s->pc += skip_width;
          if (s->pc >= PROGRAM_SIZE) s->running = false;
        }
      } else {
        while (s->pc < PROGRAM_SIZE) {
          int next = s->program[s->pc++];
          if (next == KEY_RS) break;
        }
      }
      break;
    default:
      break;
  }
}

static int read_goto_address(SantronState *s) {
  if (s->pc + 1 >= PROGRAM_SIZE) {
    s->running = false;
    return -1;
  }
  int tens = digit_from_code(s->program[s->pc]);
  int ones = digit_from_code(s->program[s->pc + 1]);
  s->pc += 2;
  if (tens < 0 || ones < 0) return -1;
  return tens * 10 + ones;
}

static int read_memory_index(SantronState *s) {
  if (s->pc >= PROGRAM_SIZE) {
    s->running = false;
    return -1;
  }
  int idx = digit_from_code(s->program[s->pc]);
  s->pc++;
  if (idx < 0) s->running = false;
  return idx;
}

static void execute_key(SantronState *s, int code, bool from_program) {
  if (from_program && code == KEY_GOTO) {
    int target = read_goto_address(s);
    if (target >= 0 && target < PROGRAM_SIZE) s->pc = target;
    else s->running = false;
    return;
  }

  if (from_program && is_memory_command(code)) {
    int idx = read_memory_index(s);
    if (idx >= 0) apply_memory_command(s, code, idx);
    return;
  }

  execute_regular_key(s, code, from_program);
}

static int run_program(SantronState *s, int max_steps) {
  s->running = true;
  s->paused = false;
  int steps = 0;
  while (s->running && steps < max_steps) {
    if (s->pc < 0 || s->pc >= PROGRAM_SIZE) {
      s->running = false;
      break;
    }
    int code = s->program[s->pc++];
    if (code != KEY_BLANK) execute_key(s, code, true);
    if (s->pc >= PROGRAM_SIZE) s->running = false;
    steps++;
  }
  return steps;
}

#ifndef SANTRON_VM_NO_MAIN
static bool load_listing(SantronState *s, const char *path) {
  FILE *fp = fopen(path, "r");
  if (!fp) {
    fprintf(stderr, "cannot open %s: %s\n", path, strerror(errno));
    return false;
  }

  char line[256];
  while (fgets(line, sizeof(line), fp)) {
    char *p = line;
    while (isspace((unsigned char)*p)) p++;
    if (*p == '\0' || *p == '#' || *p == ';') continue;

    int addr = -1;
    int code = -1;
    if (sscanf(p, "%d %d", &addr, &code) == 2) {
      if (addr >= 0 && addr < PROGRAM_SIZE) s->program[addr] = code;
    }
  }

  fclose(fp);
  s->pc = 0;
  return true;
}

static bool parse_mem_assignment(const char *text, int *idx, double *value) {
  const char *p = text;
  if (p[0] == 'M' || p[0] == 'm') p++;
  if (!isdigit((unsigned char)p[0])) return false;
  *idx = p[0] - '0';
  p++;
  if (*p != '=') return false;
  p++;
  char *end = NULL;
  errno = 0;
  *value = strtod(p, &end);
  return errno == 0 && end != p && *end == '\0' && *idx >= 0 && *idx < MEMORY_SIZE;
}

static void press_input_digit_and_run(SantronState *s, int digit, int max_steps) {
  execute_key(s, KEY_DIGIT0 + digit, false);
  run_program(s, max_steps);
}

static void dump_state(const SantronState *s) {
  printf("pc=%02d x=%.12g y=%.12g pending=%d running=%d paused=%d\n",
         s->pc, s->x, s->y, (int)s->pending, s->running ? 1 : 0, s->paused ? 1 : 0);
  for (int i = 0; i < MEMORY_SIZE; i++) {
    printf("M%d=%.12g%s", i, s->memory[i], i == MEMORY_SIZE - 1 ? "\n" : " ");
  }
}

static void usage(const char *argv0) {
  fprintf(stderr,
          "Usage: %s --program file.lst [--mem M0=1 ...] [--x value] [--pc n]\n"
          "          [--run] [--keys 3,5,1,4] [--steps n] [--dump]\n",
          argv0);
}

int main(int argc, char **argv) {
  SantronState s;
  init_state(&s);

  const char *program_path = NULL;
  const char *keys = NULL;
  int max_steps = 1000;
  bool do_run = false;
  bool do_dump = false;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--program") == 0 && i + 1 < argc) {
      program_path = argv[++i];
    } else if (strcmp(argv[i], "--mem") == 0 && i + 1 < argc) {
      int idx = -1;
      double value = 0.0;
      if (!parse_mem_assignment(argv[++i], &idx, &value)) {
        fprintf(stderr, "invalid --mem assignment\n");
        return 2;
      }
      s.memory[idx] = value;
    } else if (strcmp(argv[i], "--x") == 0 && i + 1 < argc) {
      set_x(&s, strtod(argv[++i], NULL));
    } else if (strcmp(argv[i], "--pc") == 0 && i + 1 < argc) {
      s.pc = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--steps") == 0 && i + 1 < argc) {
      max_steps = atoi(argv[++i]);
    } else if (strcmp(argv[i], "--run") == 0) {
      do_run = true;
    } else if (strcmp(argv[i], "--keys") == 0 && i + 1 < argc) {
      keys = argv[++i];
    } else if (strcmp(argv[i], "--dump") == 0) {
      do_dump = true;
    } else {
      usage(argv[0]);
      return 2;
    }
  }

  if (!program_path) {
    usage(argv[0]);
    return 2;
  }
  if (!load_listing(&s, program_path)) return 1;

  if (do_run) run_program(&s, max_steps);

  if (keys) {
    for (const char *p = keys; *p; p++) {
      if (*p == ',' || isspace((unsigned char)*p)) continue;
      if (*p < '0' || *p > '9') {
        fprintf(stderr, "invalid key digit: %c\n", *p);
        return 2;
      }
      press_input_digit_and_run(&s, *p - '0', max_steps);
    }
  }

  if (do_dump) dump_state(&s);
  else printf("%.12g\n", s.x);

  return 0;
}
#endif
