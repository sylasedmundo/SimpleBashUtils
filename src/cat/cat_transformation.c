#include "cat_common.h"

int cat_n(const cat_flags *fl, int *line_number, int start_line) {
  int n_result = 0;
  if (fl->show_line_numbers && start_line) {
    printf("%6d\t", (*line_number)++);
    n_result = 1;
  }
  return n_result;
}

int cat_b(const cat_flags *fl, int *line_number, int start_line,
          char current_char) {
  int b_result = 0;
  if (fl->not_empty_line_numbers && start_line) {
    if (current_char != '\n') {
      printf("%6d\t", (*line_number)++);
      b_result = 1;
    }
  }
  return b_result;
}

int cat_e(const cat_flags *fl, char current_char) {
  int e_result = 0;
  if (fl->show_ends && current_char == '\n') {
    printf("$");
    e_result = 1;
  }
  return e_result;
}

int cat_v(const cat_flags *fl, char current_char) {
  int v_result = 0;
  unsigned char uc = (unsigned char)current_char;

  if (fl->show_unprintable) {
    if (uc >= 128) {
      v_result = 1;
      if (uc == 255) {
        printf("M-^?");
      } else if (uc <= 159) {
        printf("M-^%c", uc - 64);
      } else {
        printf("M-%c", uc - 128);
      }
    } else if (uc == 127) {
      v_result = 1;
      printf("^?");
    } else if (uc < 32 && uc != '\t' && uc != '\n') {
      v_result = 1;
      printf("^%c", uc + 64);
    }
  }
  return v_result;
}

int cat_t(const cat_flags *fl, char current_char) {
  int t_result = 0;
  if (fl->show_tabs && current_char == '\t') {
    printf("^I");
    t_result = 1;
  }
  return t_result;
}