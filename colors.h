#ifndef COLORS_H
#define COLORS_H

#define _FG_RESET  "\33[0;97m"
#define _BG_RESET  "\33[48;0m"

#define _FG_RED    "\33[1;31m"
#define _FG_GREEN  "\33[1;32m"
#define _FG_YELLOW "\33[1;33m"
#define _FG_BLUE   "\33[1;34m"
#define _FG_CYAN   "\33[1;36m"
#define _FG_WHITE  "\33[1;37m"

#define _BG_RED    "\33[1;41m"
#define _BG_GREEN  "\33[1;42m"
#define _BG_YELLOW "\33[1;43m"
#define _BG_BLUE   "\33[1;44m"
#define _BG_CYAN   "\33[1;46m"
#define _BG_WHITE  "\33[1;47m"

#define RED(s)          \
  _FG_RED s _FG_RESET

#define GREEN(s)        \
  _FG_GREEN s _FG_RESET

#define YELLOW(s)       \
  _FG_YELLOW s _FG_RESET

#define BLUE(s)         \
  _FG_BLUE s _FG_RESET

#define CYAN(s)         \
  _FG_CYAN s _FG_RESET

#define WHITE(s)        \
  _FG_WHITE s _FG_RESET

typedef enum {
  COLOR_NONE,

  FG_RED,
  FG_GREEN,
  FG_YELLOW,
  FG_BLUE,
  FG_CYAN,
  FG_WHITE,

  BG_RED,
  BG_GREEN,
  BG_YELLOW,
  BG_BLUE,
  BG_CYAN,
  BG_WHITE,
} color_t;

// colors.c
extern char *colored(const char *str, color_t fg_color, color_t bg_color);

// console.c
extern void setfgcolor(color_t fg_color);
extern void setbgcolor(color_t bg_color);

#endif // COLORS_H
