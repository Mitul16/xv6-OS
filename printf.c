#include "stat.h"
#include "types.h"
#include "user.h"

void putc(int fd, const char c) { write(fd, &c, sizeof(c)); }

static void printint(int fd, int xx, int base, int sign) {
  static char charset[] = "0123456789abcdef";
  char buf[16];
  int i;
  uint x;

  if (sign && (sign = xx < 0)) {
    x = -xx;
  } else {
    x = xx;
  }

  i = 0;
  do {
    buf[i++] = charset[x % base];
  } while ((x /= base) != 0);

  if (sign)
    buf[i++] = '-';

  while (--i >= 0)
    putc(fd, buf[i]);
}

// Print to the given fd. Only understands %d, %x, %p, %s.
void printf(int fd, const char *fmt, ...) {
  char *s;
  int c, i, state = 0;
  uint *ap = (uint *)(void *)&fmt + 1;

  for (i = 0; fmt[i]; i++) {
    c = fmt[i] & 0xff;

    if (state == 0) {
      if (c == '%') {
        state = '%';
      } else {
        putc(fd, c);
      }
    } else if (state == '%') {
      if (c == 'd') {
        printint(fd, *ap, 10, 1);
        ap++;
      } else if (c == 'x' || c == 'p') {
        printint(fd, *ap, 16, 0);
        ap++;
      } else if (c == 's') {
        s = (char *)*ap;
        ap++;

        if (s == 0)
          s = "(null)";

        while (*s != 0) {
          putc(fd, *s);
          s++;
        }
      } else if (c == 'c') {
        putc(fd, *ap);
        ap++;
      } else if (c == '%') {
        putc(fd, c);
      } else {
        // Unknown % sequence. Print it to draw attention.
        putc(fd, '%');
        putc(fd, c);
      }

      state = 0;
    }
  }
}

// Print to the given fd. Only understands %d, %x, %p, %s, %c
// (%c is already present in the code)
// Buffered-printf, saves string to memory and then writes
// to the fd when bufsize is reached or string ends
#define bprintf_BUFSIZE 128

static void bputc(int fd, char c, char *pbuf, int *bi) {
  pbuf[(*bi)++] = c;

  if (*bi >= bprintf_BUFSIZE) {
    write(fd, pbuf, bprintf_BUFSIZE);
    memset(pbuf, 0, bprintf_BUFSIZE);
    *bi = 0;
  }
}

static void bprintint(int fd, int xx, int base, int sign, char *pbuf, int *bi) {
  static char charset[] = "0123456789abcdef";
  char buf[16];
  int i;
  uint x;

  if (sign && (sign = xx < 0)) {
    x = -xx;
  } else {
    x = xx;
  }

  i = 0;
  do {
    buf[i++] = charset[x % base];
  } while ((x /= base) != 0);

  if (sign)
    buf[i++] = '-';

  while (--i >= 0)
    bputc(fd, buf[i], pbuf, bi);
}

void bprintf(int fd, const char *fmt, ...) {
  char pbuf[bprintf_BUFSIZE];
  int bi = 0;

  char *s;
  int c, i, state = 0;
  uint *ap = (uint *)(void *)&fmt + 1;

  for (i = 0; fmt[i]; i++) {
    c = fmt[i] & 0xff;

    if (state == 0) {
      if (c == '%') {
        state = '%';
      } else {
        bputc(fd, c, pbuf, &bi);
      }
    } else if (state == '%') {
      if (c == 'd') {
        bprintint(fd, *ap, 10, 1, pbuf, &bi);
        ap++;
      } else if (c == 'x' || c == 'p') {
        bprintint(fd, *ap, 16, 0, pbuf, &bi);
        ap++;
      } else if (c == 's') {
        s = (char *)*ap;
        ap++;

        if (s == 0)
          s = "(null)";

        while (*s != 0) {
          bputc(fd, *s, pbuf, &bi);
          s++;
        }
      } else if (c == 'c') {
        bputc(fd, *ap, pbuf, &bi);
        ap++;
      } else if (c == '%') {
        bputc(fd, c, pbuf, &bi);
      } else {
        // Unknown % sequence.  Print it to draw attention.
        bputc(fd, '%', pbuf, &bi);
        bputc(fd, c, pbuf, &bi);
      }

      state = 0;
    }
  }

  if (bi > 0) {
    write(fd, pbuf, bi);
  }
}

#undef bprintf_BUFSIZE
