#include "cat_common.h"

int main(int argc, char *argv[]) {
  int result = EXIT_SUCCESS;
  cat_flags fl;

  init_flags(&fl);
  if (flag_parsing(argc, argv, &fl) != EXIT_SUCCESS) {
    result = EXIT_FAILURE;
  } else {
    result = cat_output(argc, (const char **)argv, &fl);
  }
  return result;
}

void init_flags(cat_flags *fl) {
  fl->show_line_numbers = 0;
  fl->not_empty_line_numbers = 0;
  fl->show_ends = 0;
  fl->show_unprintable = 0;
  fl->show_tabs = 0;
  fl->squeeze_blank = 0;
}

int flag_parsing(int argc, char *argv[], cat_flags *fl) {
  int result = EXIT_SUCCESS;
  int opt;
  opterr = 0;
  struct option long_options[] = {{"number", no_argument, NULL, 'n'},
                                  {"number-nonblank", no_argument, NULL, 'b'},
                                  {"squeeze-blank", no_argument, NULL, 's'},
                                  {NULL, 0, NULL, 0}};

  while ((opt = getopt_long(argc, argv, "bnEevtTs", long_options, NULL)) !=
         -1) {
    switch (opt) {
      case 'n':
        if (!fl->not_empty_line_numbers) {
          fl->show_line_numbers = 1;
        }
        break;
      case 'b':
        fl->not_empty_line_numbers = 1;
        fl->show_line_numbers = 0;
        break;
      case 'e':
        fl->show_ends = 1;
        fl->show_unprintable = 1;
        break;
      case 'E':
        fl->show_ends = 1;
        break;
      case 'v':
        fl->show_unprintable = 1;
        break;
      case 't':
        fl->show_tabs = 1;
        fl->show_unprintable = 1;
        break;
      case 'T':
        fl->show_tabs = 1;
        break;
      case 's':
        fl->squeeze_blank = 1;
        break;
      case '?':
        fprintf(stderr, "s21_cat: invalid option -- '%c'\n", optopt);
        result = EXIT_FAILURE;
        break;
    }
  }
  return result;
}

int cat_file_process(FILE *file, cat_flags *fl, int *line_number,
                     int *last_processed_char) {
  int start_line = 1;
  int c;
  int consecutive_blank = 0;
  while ((c = fgetc(file)) != EOF) {
    int char_printed = 0;
    if (fl->squeeze_blank) {
      if (c == '\n') {
        if (consecutive_blank > 1) {
          consecutive_blank++;
          *last_processed_char = c;
          continue;
        }
        consecutive_blank++;
      } else {
        consecutive_blank = 0;
      }
    }
    if (*last_processed_char == '\n') {
      if (fl->not_empty_line_numbers) {
        cat_b(fl, line_number, start_line, c);
      } else if (fl->show_line_numbers) {
        cat_n(fl, line_number, start_line);
      }
    }
    if (fl->show_ends && c == '\n') {
      char_printed = cat_e(fl, c);
    }
    if (fl->show_tabs && c == '\t') {
      char_printed = cat_t(fl, c);
    } else if (fl->show_unprintable || fl->show_tabs) {
      char_printed = cat_v(fl, c);
    }
    if (!char_printed) {
      putchar(c);
    }
    start_line = (c == '\n');
    *last_processed_char = c;
  }
  return EXIT_SUCCESS;
}

int cat_output(int argc, const char *argv[], cat_flags *fl) {
  int result = EXIT_SUCCESS;
  FILE *file = NULL;
  int line_number = 1;
  int last_processed_char = '\n';
  if (optind >= argc) {
    printf("Usage: %s [OPTION] <filename>\n", argv[0]);
    result = EXIT_FAILURE;
  }
  if (result == EXIT_SUCCESS) {
    for (int i = optind; i < argc; i++) {
      file = fopen(argv[i], "r");
      if (file == NULL) {
        perror("Failed to open file");
        result = EXIT_FAILURE;
        continue;
      }
      cat_file_process(file, fl, &line_number, &last_processed_char);
      fclose(file);
    }
  }
  return result;
}
