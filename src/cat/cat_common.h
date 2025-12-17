#ifndef CAT_COMMON_H
#define CAT_COMMON_H

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

// cat_flags
typedef struct cat_options {
  int show_line_numbers;       // n
  int not_empty_line_numbers;  // b
  int show_ends;               // e
  int show_unprintable;        // v
  int show_tabs;               // t
  int squeeze_blank;           // s
} cat_flags;

// cat_transformation.c functions
int cat_n(const cat_flags *fl, int *line_number, int start_line);
int cat_b(const cat_flags *fl, int *line_number, int start_line,
          char current_char);
int cat_e(const cat_flags *fl, char current_char);
int cat_v(const cat_flags *fl, char current_char);
int cat_t(const cat_flags *fl, char current_char);

// s21_cat.c functions
void init_flags(cat_flags *fl);
int flag_parsing(int argc, char *argv[], cat_flags *fl);
int cat_output(int argc, const char *argv[], cat_flags *fl);
int cat_file_process(FILE *file, cat_flags *fl, int *line_number,
                     int *last_processed_char);

#endif