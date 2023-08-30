#include "colors.h"
#include "user.h"

static char *__strcpy(char *s, const char *t) {
  char *os = s;
  while ((*s++ = *t++) != 0)
    ;

  return os;
}

static unsigned int __strlen(const char *s) {
  unsigned int n;
  for (n = 0; s[n]; n++)
    ;

  return n;
}

static char *__strcat(char *s, const char *t) {
  char *os = s;
  s = s + __strlen(s);

  while (*t != 0)
    *s++ = *t++;

  *s = 0;
  return os;
}

char *colored(const char *str, color_t fg_color, color_t bg_color) {
  static char _temp[256];
  __strcpy(_temp, "");

  switch (bg_color) {
  case BG_RED:
    __strcat(_temp, _BG_RED);
    break;

  case BG_GREEN:
    __strcat(_temp, _BG_GREEN);
    break;

  case BG_YELLOW:
    __strcat(_temp, _BG_YELLOW);
    break;

  case BG_BLUE:
    __strcat(_temp, _BG_BLUE);
    break;

  case BG_CYAN:
    __strcat(_temp, _BG_CYAN);
    break;

  case BG_WHITE:
    __strcat(_temp, _BG_WHITE);
    break;

  default:
    break;
  }

  switch (fg_color) {
  case FG_RED:
    __strcat(_temp, _FG_RED);
    break;

  case FG_GREEN:
    __strcat(_temp, _FG_GREEN);
    break;

  case FG_YELLOW:
    __strcat(_temp, _FG_YELLOW);
    break;

  case FG_BLUE:
    __strcat(_temp, _FG_BLUE);
    break;

  case FG_CYAN:
    __strcat(_temp, _FG_CYAN);
    break;

  case FG_WHITE:
    __strcat(_temp, _FG_WHITE);
    break;

  default:
    break;
  }

  __strcat(_temp, str);
  __strcat(_temp, _FG_RESET);
  __strcat(_temp, _BG_RESET);

  return _temp;
}
