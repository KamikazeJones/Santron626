#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define SANTRON_VM_NO_MAIN
#include "santron-vm.c"

static bool nearly_equal(double a, double b) {
  return fabs(a - b) < 1e-9;
}

static void load_candidate(SantronState *s, const int *codes, int code_count) {
  for (int i = 0; i < PROGRAM_SIZE; i++) s->program[i] = KEY_BLANK;
  for (int i = 0; i < code_count && i < PROGRAM_SIZE; i++) s->program[i] = codes[i];
  if (code_count < PROGRAM_SIZE) s->program[code_count] = KEY_RS;
  s->pc = 0;
}

static bool check_miss_decrement_target(const int *codes, int code_count, bool verbose) {
  int failures = 0;

  for (int m7 = 0; m7 <= 4; m7++) {
    for (int m9 = 0; m9 <= 4; m9++) {
      SantronState s;
      init_state(&s);
      load_candidate(&s, codes, code_count);

      s.x = 123.0;
      s.y = 0.0;
      s.pending = OP_NONE;
      s.memory[0] = 1.0;
      s.memory[7] = (double)m7;
      s.memory[9] = (double)m9;

      run_program(&s, 200);

      double expected_m9 = (double)(m9 - (m7 == 0 ? 1 : 0));
      bool ok = true;
      ok = ok && nearly_equal(s.memory[0], 1.0);
      ok = ok && nearly_equal(s.memory[7], (double)m7);
      ok = ok && nearly_equal(s.memory[9], expected_m9);
      ok = ok && s.pending == OP_NONE;
      ok = ok && s.pending_memory < 0;
      ok = ok && !s.exponent_entry;

      if (!ok) {
        failures++;
        if (verbose) {
          printf("FAIL M7=%d M9=%d -> got M7=%.12g M9=%.12g pending=%d pending_memory=%d x=%.12g pc=%d\n",
                 m7, m9, s.memory[7], s.memory[9], (int)s.pending,
                 s.pending_memory, s.x, s.pc);
        }
      }
    }
  }

  return failures == 0;
}

static int parse_codes(int argc, char **argv, int *codes, int max_codes) {
  int count = 0;
  for (int i = 1; i < argc; i++) {
    if (count >= max_codes) {
      fprintf(stderr, "too many key codes\n");
      exit(2);
    }
    char *end = NULL;
    long code = strtol(argv[i], &end, 10);
    if (end == argv[i] || *end != '\0') {
      fprintf(stderr, "invalid key code: %s\n", argv[i]);
      exit(2);
    }
    codes[count++] = (int)code;
  }
  return count;
}

int main(int argc, char **argv) {
  int codes[PROGRAM_SIZE];
  int count = 0;

  if (argc == 1) {
    const int known_miss_block[] = {
      KEY_RCL, KEY_DIGIT0 + 7,
      KEY_SQR,
      KEY_SIGN,
      KEY_SKP,
      KEY_RCL, KEY_DIGIT0 + 0,
      KEY_MMINUS, KEY_DIGIT0 + 9
    };
    count = (int)(sizeof(known_miss_block) / sizeof(known_miss_block[0]));
    for (int i = 0; i < count; i++) codes[i] = known_miss_block[i];
  } else {
    count = parse_codes(argc, argv, codes, PROGRAM_SIZE);
  }

  bool ok = check_miss_decrement_target(codes, count, true);
  printf("%s miss-decrement target (%d cells)\n", ok ? "PASS" : "FAIL", count);
  return ok ? 0 : 1;
}
