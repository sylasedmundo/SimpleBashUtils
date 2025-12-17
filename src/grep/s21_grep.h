#ifndef S21_GREP_H
#define S21_GREP_H
#define _POSIX_C_SOURCE 200809L
#include <getopt.h>
#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_PATTERNS 100
#define MAX_LINE_LENGTH 8192

struct grep_options {
  bool ignore_case;
  bool invert_match;
  bool count_only;
  bool filename_only;
  bool line_number;
  bool no_filename;
  bool suppress_errors;
  bool only_matching;
  bool multiple_files;
  char *pattern_file;
  char *patterns[MAX_PATTERNS];
  int pattern_count;
};

struct grep_state {
  regex_t regex;
  long line_counter;
  int match_count;
  int file_match_count;
  char current_file[256];
};

void init_options(struct grep_options *opt);
void add_pattern(struct grep_options *opt, const char *pattern);
void escape_pattern_for_combination(char *dest, const char *src,
                                    size_t dest_size);
int compile_patterns(struct grep_options *opt, struct grep_state *state);
void free_patterns(struct grep_options *opt);
void load_patterns_from_file(struct grep_options *opt, const char *filename);
void print_match(const struct grep_options *opt, const struct grep_state *state,
                 const char *line, regmatch_t *matches);
int process_stream(FILE *input, const struct grep_options *opt,
                   struct grep_state *state);
void print_count(const struct grep_options *opt,
                 const struct grep_state *state);
int parse_arguments(int argc, char *argv[], struct grep_options *opt);
int process_files(int argc, const char *argv[], const struct grep_options *opt,
                  struct grep_state *state);
#endif