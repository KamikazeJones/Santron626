#include <ctype.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SANTRON_VM_NO_MAIN
#include "santron-vm.c"

static bool load_listing_file(SantronState *s, const char *path) {
  FILE *fp = fopen(path, "r");
  if (!fp) {
    perror(path);
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

static bool find_output_pause_pcs(const SantronState *s, int *black_pc, int *white_pc) {
  int previous = -1;
  int last = -1;

  for (int i = 0; i < PROGRAM_SIZE; i++) {
    if (s->program[i] == KEY_RS) {
      previous = last;
      last = i;
    }
  }

  if (previous < 0 || last < 0) return false;
  *black_pc = previous + 1;
  *white_pc = last + 1;
  return true;
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

static bool close_to(double actual, int expected) {
  return fabs(actual - (double)expected) <= 1e-8;
}

static void print_digits(const int digits[4]) {
  for (int i = 0; i < 4; i++) printf("%d", digits[i]);
}

int main(int argc, char **argv) {
  const char *program_path = argc > 1 ? argv[1] : "mastermind-14.lst";

  SantronState base;
  init_state(&base);
  if (!load_listing_file(&base, program_path)) return 2;

  int black_pause_pc = -1;
  int white_pause_pc = -1;
  if (!find_output_pause_pcs(&base, &black_pause_pc, &white_pause_pc)) {
    fprintf(stderr, "cannot find output R/S instructions in %s\n", program_path);
    return 2;
  }

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

                  int expected_black = 0;
                  int expected_white = 0;
                  expected_score(code, guess, &expected_black, &expected_white);

                  SantronState s = base;
                  s.memory[0] = 1.0;
                  s.memory[1] = 0.0;
                  s.memory[2] = 1.0;
                  s.memory[8] = (double)encode_code(code);
                  s.memory[9] = 4.0;

                  run_program(&s, 1000);
                  for (int i = 0; i < 4; i++) {
                    execute_key(&s, KEY_DIGIT0 + guess[i], false);
                    run_program(&s, 1000);
                  }

                  bool ok = true;
                  ok = ok && s.paused && s.pc == black_pause_pc;
                  ok = ok && close_to(s.x, expected_black);
                  ok = ok && close_to(s.memory[1], expected_black);
                  ok = ok && close_to(s.memory[9], expected_white);

                  run_program(&s, 1000);
                  ok = ok && s.paused && s.pc == white_pause_pc;
                  ok = ok && close_to(s.x, expected_white);

                  if (!ok) {
                    printf("FAIL code=");
                    print_digits(code);
                    printf(" guess=");
                    print_digits(guess);
                    printf(" expected=%d/%d got x=%.12g M1=%.12g M9=%.12g pc=%d paused=%d\n",
                           expected_black, expected_white, s.x, s.memory[1], s.memory[9],
                           s.pc, s.paused ? 1 : 0);
                    return 1;
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

  printf("PASS %d code/guess combinations from digits 1..6 without repeats; output PCs black=%d white=%d\n",
         tested, black_pause_pc, white_pause_pc);
  return 0;
}
