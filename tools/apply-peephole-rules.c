#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_RULES 256
#define MAX_PATTERN 512
#define MAX_REPLACEMENT 256
#define MAX_LINE 1024
#define MAX_PROGRAM 100
#define MAX_STRING 4096
#define MAX_GROUPS 10

typedef struct {
  char regex_text[MAX_PATTERN];
  char replacement[MAX_REPLACEMENT];
  regex_t regex;
} Rule;

typedef struct {
  char name[16];
  int group;
} VarBinding;

static int key_code(const char *token) {
  if (strlen(token) == 1 && token[0] >= '0' && token[0] <= '9') return 100 + token[0] - '0';
  if (strcmp(token, "+") == 0) return 10;
  if (strcmp(token, "-") == 0) return 11;
  if (strcmp(token, "*") == 0) return 12;
  if (strcmp(token, "/") == 0) return 13;
  if (strcmp(token, "Y^X") == 0) return 14;
  if (strcmp(token, "1/X") == 0) return 17;
  if (strcmp(token, "X^2") == 0) return 18;
  if (strcmp(token, "SQRT") == 0) return 19;
  if (strcmp(token, "SIN") == 0) return 20;
  if (strcmp(token, "COS") == 0) return 21;
  if (strcmp(token, "TAN") == 0) return 22;
  if (strcmp(token, "ASIN") == 0) return 30;
  if (strcmp(token, "ACOS") == 0) return 31;
  if (strcmp(token, "ATAN") == 0) return 32;
  if (strcmp(token, "LN") == 0) return 40;
  if (strcmp(token, "LOG") == 0) return 41;
  if (strcmp(token, "E^X") == 0) return 42;
  if (strcmp(token, "10^X") == 0) return 43;
  if (strcmp(token, "M+") == 0) return 50;
  if (strcmp(token, "M-") == 0) return 51;
  if (strcmp(token, "M*") == 0) return 52;
  if (strcmp(token, "M/") == 0) return 53;
  if (strcmp(token, "STO") == 0) return 55;
  if (strcmp(token, "RCL") == 0) return 56;
  if (strcmp(token, "C/CE") == 0) return 60;
  if (strcmp(token, "+/-") == 0) return 62;
  if (strcmp(token, "EXP") == 0) return 63;
  if (strcmp(token, "X<>Y") == 0) return 64;
  if (strcmp(token, "PI") == 0) return 65;
  if (strcmp(token, ".") == 0) return 70;
  if (strcmp(token, "PS") == 0) return 71;
  if (strcmp(token, "=") == 0) return 80;
  if (strcmp(token, "(") == 0) return 81;
  if (strcmp(token, ")") == 0) return 82;
  if (strcmp(token, "R/S") == 0) return 90;
  if (strcmp(token, "GOTO") == 0) return 93;
  if (strcmp(token, "SKP") == 0) return 96;
  if (strcmp(token, "blank") == 0) return 99;
  return -1;
}

static const char *code_name(int code) {
  switch (code) {
    case 10: return "+";
    case 11: return "-";
    case 12: return "*";
    case 13: return "/";
    case 14: return "Y^X";
    case 17: return "1/X";
    case 18: return "X^2";
    case 19: return "SQRT";
    case 20: return "SIN";
    case 21: return "COS";
    case 22: return "TAN";
    case 30: return "ASIN";
    case 31: return "ACOS";
    case 32: return "ATAN";
    case 40: return "LN";
    case 41: return "LOG";
    case 42: return "E^X";
    case 43: return "10^X";
    case 50: return "M+";
    case 51: return "M-";
    case 52: return "M*";
    case 53: return "M/";
    case 55: return "STO";
    case 56: return "RCL";
    case 60: return "C/CE";
    case 62: return "+/-";
    case 63: return "EXP";
    case 64: return "X<>Y";
    case 65: return "PI";
    case 70: return ".";
    case 71: return "PS";
    case 80: return "=";
    case 81: return "(";
    case 82: return ")";
    case 90: return "R/S";
    case 93: return "GOTO";
    case 96: return "SKP";
    case 99: return "";
    default: break;
  }
  static char digits[10][2] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};
  if (code >= 100 && code <= 109) return digits[code - 100];
  return "?";
}

static void trim(char *text) {
  char *start = text;
  while (isspace((unsigned char)*start)) start++;
  if (start != text) memmove(text, start, strlen(start) + 1);
  size_t len = strlen(text);
  while (len > 0 && isspace((unsigned char)text[len - 1])) text[--len] = '\0';
}

static void appendf(char *out, size_t out_size, const char *text) {
  strncat(out, text, out_size - strlen(out) - 1);
}

static int find_binding(VarBinding *bindings, int count, const char *name) {
  for (int i = 0; i < count; i++) {
    if (strcmp(bindings[i].name, name) == 0) return i;
  }
  return -1;
}

static void compile_side_token(const char *token, bool lhs, VarBinding *bindings,
                               int *binding_count, int *group_count,
                               char *out, size_t out_size) {
  if (token[0] == '$') {
    int idx = find_binding(bindings, *binding_count, token);
    if (lhs) {
      if (idx < 0) {
        idx = (*binding_count)++;
        snprintf(bindings[idx].name, sizeof(bindings[idx].name), "%s", token);
        bindings[idx].group = ++(*group_count);
        if (strcmp(token, "$m") == 0 || strcmp(token, "$d") == 0) {
          appendf(out, out_size, "\\(10[0-9]\\)");
        } else {
          appendf(out, out_size, "\\([0-9][0-9][0-9]\\)");
        }
      } else {
        char ref[16];
        snprintf(ref, sizeof(ref), "\\%d", bindings[idx].group);
        appendf(out, out_size, ref);
      }
    } else {
      if (idx < 0) {
        fprintf(stderr, "unbound rhs variable: %s\n", token);
        exit(2);
      }
      char ref[16];
      snprintf(ref, sizeof(ref), "$%d", bindings[idx].group);
      appendf(out, out_size, ref);
    }
    return;
  }

  int code = key_code(token);
  if (code < 0) {
    fprintf(stderr, "unknown token in rule: %s\n", token);
    exit(2);
  }
  char buf[8];
  snprintf(buf, sizeof(buf), "%03d", code);
  appendf(out, out_size, buf);
}

static void compile_tokens(char *text, bool lhs, VarBinding *bindings,
                           int *binding_count, int *group_count,
                           char *out, size_t out_size) {
  char copy[MAX_LINE];
  snprintf(copy, sizeof(copy), "%s", text);
  char *saveptr = NULL;
  char *token = strtok_r(copy, " \t\r\n", &saveptr);
  bool first = true;
  while (token) {
    if (!first) appendf(out, out_size, " ");
    compile_side_token(token, lhs, bindings, binding_count, group_count, out, out_size);
    first = false;
    token = strtok_r(NULL, " \t\r\n", &saveptr);
  }
}

static bool compile_rule_line(const char *line, Rule *rule, int line_no) {
  char copy[MAX_LINE];
  snprintf(copy, sizeof(copy), "%s", line);
  char *comment = strchr(copy, '#');
  if (comment) *comment = '\0';
  trim(copy);
  if (!copy[0]) return false;

  char *arrow = strstr(copy, "=>");
  if (!arrow) {
    fprintf(stderr, "line %d: expected =>\n", line_no);
    exit(2);
  }
  *arrow = '\0';
  char *lhs = copy;
  char *rhs = arrow + 2;
  trim(lhs);
  trim(rhs);

  VarBinding bindings[16] = {0};
  int binding_count = 0;
  int group_count = 0;
  rule->regex_text[0] = '\0';
  rule->replacement[0] = '\0';
  appendf(rule->regex_text, sizeof(rule->regex_text), " ");
  compile_tokens(lhs, true, bindings, &binding_count, &group_count,
                 rule->regex_text, sizeof(rule->regex_text));
  appendf(rule->regex_text, sizeof(rule->regex_text), " ");
  appendf(rule->replacement, sizeof(rule->replacement), " ");
  compile_tokens(rhs, false, bindings, &binding_count, &group_count,
                 rule->replacement, sizeof(rule->replacement));
  appendf(rule->replacement, sizeof(rule->replacement), " ");

  int err = regcomp(&rule->regex, rule->regex_text, 0);
  if (err) {
    char msg[256];
    regerror(err, &rule->regex, msg, sizeof(msg));
    fprintf(stderr, "line %d: regex compile failed: %s\nregex: %s\n",
            line_no, msg, rule->regex_text);
    exit(2);
  }
  return true;
}

static int load_rules(const char *path, Rule *rules, int max_rules) {
  FILE *fp = fopen(path, "r");
  if (!fp) {
    perror(path);
    exit(1);
  }
  char line[MAX_LINE];
  int count = 0;
  int line_no = 0;
  while (fgets(line, sizeof(line), fp)) {
    line_no++;
    if (count >= max_rules) {
      fprintf(stderr, "too many rules\n");
      exit(2);
    }
    if (compile_rule_line(line, &rules[count], line_no)) count++;
  }
  fclose(fp);
  return count;
}

static int parse_listing(const char *path, int *program) {
  FILE *fp = fopen(path, "r");
  if (!fp) {
    perror(path);
    exit(1);
  }
  for (int i = 0; i < MAX_PROGRAM; i++) program[i] = 99;
  int max_addr = -1;
  char line[MAX_LINE];
  while (fgets(line, sizeof(line), fp)) {
    int addr = -1;
    int code = -1;
    if (sscanf(line, "%d %d", &addr, &code) == 2 && addr >= 0 && addr < MAX_PROGRAM) {
      program[addr] = code;
      if (addr > max_addr) max_addr = addr;
    }
  }
  fclose(fp);
  int len = max_addr + 1;
  while (len > 0 && program[len - 1] == 99) len--;
  return len;
}

static void program_to_string(const int *program, int len, char *out, size_t out_size) {
  out[0] = '\0';
  appendf(out, out_size, " ");
  for (int i = 0; i < len; i++) {
    char buf[8];
    snprintf(buf, sizeof(buf), "%s%03d", i ? " " : "", program[i]);
    appendf(out, out_size, buf);
  }
  appendf(out, out_size, " ");
}

static int string_to_program(const char *text, int *program) {
  char copy[MAX_STRING];
  snprintf(copy, sizeof(copy), "%s", text);
  int len = 0;
  char *saveptr = NULL;
  char *token = strtok_r(copy, " ", &saveptr);
  while (token && len < MAX_PROGRAM) {
    program[len++] = atoi(token);
    token = strtok_r(NULL, " ", &saveptr);
  }
  return len;
}

static void expand_replacement(const char *replacement, const char *input,
                               const regmatch_t *matches, char *out, size_t out_size) {
  out[0] = '\0';
  for (size_t i = 0; replacement[i]; i++) {
    if (replacement[i] == '$' && isdigit((unsigned char)replacement[i + 1])) {
      int group = replacement[++i] - '0';
      if (group >= 0 && group < MAX_GROUPS && matches[group].rm_so >= 0) {
        int start = matches[group].rm_so;
        int end = matches[group].rm_eo;
        char buf[64];
        int n = end - start;
        if (n >= (int)sizeof(buf)) n = (int)sizeof(buf) - 1;
        memcpy(buf, input + start, (size_t)n);
        buf[n] = '\0';
        appendf(out, out_size, buf);
      }
    } else {
      char buf[2] = {replacement[i], '\0'};
      appendf(out, out_size, buf);
    }
  }
}

static bool apply_one_rule(char *program_text, size_t program_size, const Rule *rule) {
  regmatch_t matches[MAX_GROUPS];
  int err = regexec(&rule->regex, program_text, MAX_GROUPS, matches, 0);
  if (err != 0) return false;

  int core_start = matches[0].rm_so;
  int core_end = matches[0].rm_eo;

  char replacement[MAX_REPLACEMENT];
  expand_replacement(rule->replacement, program_text, matches, replacement, sizeof(replacement));

  char next[MAX_STRING];
  next[0] = '\0';
  strncat(next, program_text, (size_t)core_start);
  appendf(next, sizeof(next), replacement);
  if (program_text[core_end]) {
    appendf(next, sizeof(next), program_text + core_end);
  }

  snprintf(program_text, program_size, "%s", next);
  return true;
}

static int apply_rules(char *program_text, size_t program_size, Rule *rules, int rule_count) {
  int replacements = 0;
  bool changed = true;
  while (changed) {
    changed = false;
    for (int i = 0; i < rule_count; i++) {
      if (apply_one_rule(program_text, program_size, &rules[i])) {
        replacements++;
        changed = true;
        break;
      }
    }
  }
  return replacements;
}

static void write_listing(FILE *out, const int *program, int len) {
  for (int i = 0; i < len; i++) {
    fprintf(out, "%02d %03d", i, program[i]);
    const char *name = code_name(program[i]);
    if (name[0]) fprintf(out, " %s", name);
    fprintf(out, "\n");
  }
}

static const char *arg_value(int argc, char **argv, const char *name, const char *fallback) {
  for (int i = 1; i + 1 < argc; i++) {
    if (strcmp(argv[i], name) == 0) return argv[i + 1];
  }
  return fallback;
}

int main(int argc, char **argv) {
  const char *rules_path = arg_value(argc, argv, "--rules", "tools/peephole-rules.txt");
  const char *program_path = arg_value(argc, argv, "--program", NULL);
  const char *out_path = arg_value(argc, argv, "--out", NULL);

  if (!program_path) {
    fprintf(stderr, "Usage: %s --rules tools/peephole-rules.txt --program file.lst [--out file.lst]\n", argv[0]);
    return 2;
  }

  Rule rules[MAX_RULES];
  int rule_count = load_rules(rules_path, rules, MAX_RULES);

  int program[MAX_PROGRAM];
  int before_len = parse_listing(program_path, program);
  char program_text[MAX_STRING];
  program_to_string(program, before_len, program_text, sizeof(program_text));
  int replacements = apply_rules(program_text, sizeof(program_text), rules, rule_count);
  int after_len = string_to_program(program_text, program);

  FILE *out = stdout;
  if (out_path) {
    out = fopen(out_path, "w");
    if (!out) {
      perror(out_path);
      return 1;
    }
  }
  write_listing(out, program, after_len);
  if (out_path) fclose(out);

  fprintf(stderr, "rules=%d replacements=%d cells=%d->%d\n",
          rule_count, replacements, before_len, after_len);

  for (int i = 0; i < rule_count; i++) regfree(&rules[i].regex);
  return 0;
}
