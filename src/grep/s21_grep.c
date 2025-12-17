#include "s21_grep.h"

int main(int argc, char *argv[]) {
  struct grep_options opt;
  struct grep_state state;
  int result = EXIT_SUCCESS;

  init_options(&opt);

  if (parse_arguments(argc, argv, &opt) == 0) {
    if (compile_patterns(&opt, &state) == 0) {
      opt.multiple_files = (argc - optind > 1);
      state.match_count = 0;
      result = process_files(argc, (const char **)argv, &opt, &state);
      regfree(&state.regex);
    } else {
      if (!opt.suppress_errors) {
        fprintf(stderr, "grep: invalid pattern\n");
      }
      result = EXIT_FAILURE;
    }
  } else {
    if (!opt.suppress_errors) {
      fprintf(stderr, "grep: pattern required\n");
    }
    result = EXIT_FAILURE;
  }

  free_patterns(&opt);
  return result;
}

void init_options(struct grep_options *opt) {
  opt->ignore_case = false;
  opt->invert_match = false;
  opt->count_only = false;
  opt->filename_only = false;
  opt->line_number = false;
  opt->no_filename = false;
  opt->suppress_errors = false;
  opt->only_matching = false;
  opt->multiple_files = false;
  opt->pattern_file = NULL;
  opt->pattern_count = 0;
}

void add_pattern(struct grep_options *opt, const char *pattern) {
  if (opt->pattern_count < MAX_PATTERNS && pattern != NULL) {
    char *dup = strdup(pattern);
    if (dup != NULL) {
      opt->patterns[opt->pattern_count] = dup;
      opt->pattern_count++;
    }
  }
}

void escape_pattern_for_combination(char *dest, const char *src,
                                    size_t dest_size) {
  size_t j = 0;

  if (src == NULL || strlen(src) == 0) {
    if (dest_size > 0) {
      strcpy(dest, "^$");
    }
    return;
  }
  if (strchr(src, '|') != NULL || (src[0] == '^' && strlen(src) > 1)) {
    strncpy(dest, src, dest_size - 1);
    dest[dest_size - 1] = '\0';
    return;
  }

  for (size_t i = 0; src[i] != '\0' && j < dest_size - 2; i++) {
    if (src[i] == '.' || src[i] == '\\') {
      if (j < dest_size - 2) {
        dest[j++] = '\\';
        dest[j++] = src[i];
      }
    } else {
      dest[j++] = src[i];
    }
  }
  dest[j] = '\0';
}

int compile_patterns(struct grep_options *opt, struct grep_state *state) {
  int result = -1;
  int flags = REG_EXTENDED;
  if (opt->ignore_case) flags |= REG_ICASE;

  if (opt->pattern_count == 0) {
    result = regcomp(&state->regex, "^$", flags);
  } else if (opt->pattern_count == 1) {
    const char *pattern = opt->patterns[0];
    const char *final_pattern =
        (pattern == NULL || strlen(pattern) == 0) ? "^$" : pattern;
    result = regcomp(&state->regex, final_pattern, flags);
  } else {
    char combined_pattern[8192] = "";
    size_t current_len = 0;
    bool first_pattern = true;

    for (int i = 0; i < opt->pattern_count; i++) {
      const char *pattern = opt->patterns[i];
      char processed_pattern[256];
      const char *final_pattern;

      if (pattern != NULL && strlen(pattern) > 0) {
        escape_pattern_for_combination(processed_pattern, pattern,
                                       sizeof(processed_pattern));
        final_pattern = processed_pattern;
      } else {
        final_pattern = "^$";
      }

      size_t pattern_len = strlen(final_pattern);

      if (!first_pattern) {
        if (current_len + pattern_len + 2 >= sizeof(combined_pattern)) break;
        strcat(combined_pattern, "|");
        current_len++;
      }

      if (current_len + pattern_len + 1 < sizeof(combined_pattern)) {
        strcat(combined_pattern, final_pattern);
        current_len += pattern_len;
      }

      first_pattern = false;
    }
    result = regcomp(&state->regex, combined_pattern, flags);
  }

  return result;
}

void free_patterns(struct grep_options *opt) {
  for (int i = 0; i < opt->pattern_count; i++) {
    if (opt->patterns[i] != NULL) {
      free(opt->patterns[i]);
    }
  }
  if (opt->pattern_file) {
    free(opt->pattern_file);
  }
}

void load_patterns_from_file(struct grep_options *opt, const char *filename) {
  FILE *file = fopen(filename, "r");
  if (!file) {
    if (!opt->suppress_errors) {
      fprintf(stderr, "grep: %s: Cannot open file\n", filename);
    }
    return;
  }

  char line[MAX_LINE_LENGTH];
  int patterns_loaded = 0;

  while (fgets(line, sizeof(line), file)) {
    size_t len = strlen(line);
    if (len > 0 && line[len - 1] == '\n') {
      line[len - 1] = '\0';
    }
    add_pattern(opt, line);
    patterns_loaded++;
  }

  if (patterns_loaded == 0) {
    add_pattern(opt, "");
  }

  fclose(file);
}

void print_match(const struct grep_options *opt, const struct grep_state *state,
                 const char *line, regmatch_t *matches) {
  if (opt->filename_only) {
    printf("%s\n", state->current_file);
  } else if (!opt->count_only) {
    if (!opt->no_filename && opt->multiple_files) {
      printf("%s:", state->current_file);
    }

    if (opt->line_number) {
      printf("%ld:", state->line_counter);
    }

    if (opt->only_matching && matches != NULL) {
      for (int i = 0; i < 10; i++) {
        if (matches[i].rm_so != -1) {
          int start = matches[i].rm_so;
          int end = matches[i].rm_eo;
          if (end > start) {
            printf("%.*s\n", end - start, line + start);
          }
        }
      }
    } else if (!(opt->invert_match && opt->only_matching)) {
      printf("%s", line);
      if (line[strlen(line) - 1] != '\n') printf("\n");
    }
  }
}

int process_stream(FILE *input, const struct grep_options *opt,
                   struct grep_state *state) {
  char line[MAX_LINE_LENGTH];
  state->line_counter = 0;
  state->file_match_count = 0;

  bool output_enabled = !opt->count_only && !opt->filename_only;

  while (fgets(line, sizeof(line), input)) {
    state->line_counter++;

    int regex_result;
    regmatch_t matches[10];

    if (opt->only_matching && !opt->invert_match && output_enabled) {
      const char *search_ptr = line;
      bool found_any = false;

      while (search_ptr != NULL && *search_ptr != '\0') {
        regex_result = regexec(&state->regex, search_ptr, 10, matches, 0);
        if (regex_result != 0) break;

        if (matches[0].rm_so != -1 && matches[0].rm_eo != -1) {
          found_any = true;
          state->file_match_count++;
          state->match_count++;

          if (!opt->no_filename && opt->multiple_files) {
            printf("%s:", state->current_file);
          }
          if (opt->line_number) {
            printf("%ld:", state->line_counter);
          }

          int start = matches[0].rm_so;
          int end = matches[0].rm_eo;
          printf("%.*s\n", end - start, search_ptr + start);

          search_ptr += matches[0].rm_eo;
          if (matches[0].rm_so == matches[0].rm_eo) {
            search_ptr++;
          }
        } else {
          break;
        }
      }

      if (found_any) continue;
    } else {
      regex_result = regexec(&state->regex, line, 10, matches, 0);
      bool found = (regex_result == 0);
      bool should_print = found != opt->invert_match;

      if (should_print) {
        state->file_match_count++;
        state->match_count++;

        if (output_enabled) {
          if (opt->only_matching) {
            if (!opt->invert_match) {
              print_match(opt, state, line, matches);
            }
          } else {
            print_match(opt, state, line, NULL);
          }
        }

        if (opt->filename_only) {
          break;
        }
      }
    }
  }

  return 0;
}

void print_count(const struct grep_options *opt,
                 const struct grep_state *state) {
  if (opt->filename_only) {
    if (state->file_match_count > 0) {
      printf("%s\n", state->current_file);
    }
  } else if (opt->count_only) {
    if (!opt->no_filename && opt->multiple_files) {
      printf("%s:", state->current_file);
    }
    printf("%d\n", state->file_match_count);
  }
}

int parse_arguments(int argc, char *argv[], struct grep_options *opt) {
  int optch;
  bool pattern_specified = false;

  while ((optch = getopt(argc, argv, "e:ivclnhsf:o")) != -1) {
    switch (optch) {
      case 'e':
        add_pattern(opt, optarg);
        pattern_specified = true;
        break;
      case 'i':
        opt->ignore_case = true;
        break;
      case 'v':
        opt->invert_match = true;
        break;
      case 'c':
        opt->count_only = true;
        break;
      case 'l':
        opt->filename_only = true;
        break;
      case 'n':
        opt->line_number = true;
        break;
      case 'h':
        opt->no_filename = true;
        break;
      case 's':
        opt->suppress_errors = true;
        break;
      case 'f':
        load_patterns_from_file(opt, optarg);
        pattern_specified = true;
        break;
      case 'o':
        opt->only_matching = true;
        break;
      default:
        return 1;
    }
  }

  for (int i = 1; i < argc; i++) {
    if (argv[i] != NULL && argv[i][0] == '-' && argv[i][1] == 'e' &&
        argv[i][2] != '\0') {
      add_pattern(opt, argv[i] + 2);
      pattern_specified = true;
      argv[i] = NULL;
    }
  }

  if (!pattern_specified) {
    for (int i = optind; i < argc; i++) {
      if (argv[i] != NULL && argv[i][0] != '-') {
        add_pattern(opt, argv[i]);
        optind = i + 1;
        pattern_specified = true;
        break;
      }
    }
  }

  return pattern_specified ? 0 : 1;
}

int process_files(int argc, const char *argv[], const struct grep_options *opt,
                  struct grep_state *state) {
  int result = EXIT_SUCCESS;

  if (optind == argc) {
    // stdin
    strncpy(state->current_file, "(standard input)",
            sizeof(state->current_file) - 1);
    state->current_file[sizeof(state->current_file) - 1] = '\0';
    process_stream(stdin, opt, state);

    if (opt->count_only || opt->filename_only) {
      print_count(opt, state);
    }
  } else {
    for (int i = optind; i < argc; i++) {
      if (argv[i] == NULL) continue;

      FILE *file;

      state->file_match_count = 0;

      if (strcmp(argv[i], "-") == 0) {
        file = stdin;
        strncpy(state->current_file, "(standard input)",
                sizeof(state->current_file) - 1);
      } else {
        file = fopen(argv[i], "r");
        if (!file) {
          if (!opt->suppress_errors) {
            fprintf(stderr, "grep: %s: Cannot open file\n", argv[i]);
          }
          result = EXIT_FAILURE;
          continue;
        }
        strncpy(state->current_file, argv[i], sizeof(state->current_file) - 1);
      }
      state->current_file[sizeof(state->current_file) - 1] = '\0';

      process_stream(file, opt, state);

      if (opt->count_only || opt->filename_only) {
        print_count(opt, state);
      }

      if (file != stdin) {
        fclose(file);
      }
    }
  }

  return result;
}
