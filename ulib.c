#include "fcntl.h"
#include "stat.h"
#include "types.h"
#include "user.h"
#include "x86.h"

extern void *malloc(uint);
extern void free(void *);

char *strcpy(char *s, const char *t) {
  char *os = s;
  while ((*s++ = *t++) != 0)
    ;

  return os;
}

int strcmp(const char *p, const char *q) {
  while (*p && *p == *q)
    p++, q++;

  return (uchar)*p - (uchar)*q;
}

uint strlen(const char *s) {
  int n;
  for (n = 0; s[n]; n++)
    ;

  return n;
}

void *memset(void *dst, int c, uint n) {
  stosb(dst, c, n);
  return dst;
}

char *strchr(const char *s, char c) {
  for (; *s; s++)
    if (*s == c)
      return (char *)s;

  return 0;
}

/*char*
gets(char *buf, int max)
{
  int i, cc;
  char c;

  for (i = 0; i+1 < max; ) {
    cc = read(0, &c, 1);
    if (cc < 1)
      break;

    buf[i++] = c;
    if (c == '\n' || c == '\r')
      break;
  }

  buf[i] = '\0';
  return buf;
}*/

// simpler
char *gets(char *buf, int max, char delim) {
  char *os = buf;
  while ((buf - os) < max && read(0, buf, 1) > 0 && *buf && *buf != delim)
    buf++;

  *buf = '\0';
  return os;
}

/*
 * this is not valid, we now have SYMLINKs
 * and it performs `open(..., O_RDONLY)` followed by `fstat(...)`
 * `O_NOFOLLOW` needs to be implemented into the `stat` method
 */

/*
int stat(const char *n, struct stat *st) {
  int fd;
  int r;

  fd = open(n, O_RDONLY);
  if (fd < 0)
    return -1;

  r = fstat(fd, st);
  close(fd);
  return r;
}
*/

int atoi(const char *s) {
  int n;

  n = 0;
  while ('0' <= *s && *s <= '9')
    n = n * 10 + *s++ - '0';

  return n;
}

void *memmove(void *vdst, const void *vsrc, int n) {
  char *temp = (char *)malloc(n);
  char *p = temp;

  if (!temp)
    return 0;

  int m = n;

  while (m-- > 0)
    *p++ = *((char *)vsrc++);

  p = temp;

  while (n-- > 0)
    *((char *)vdst++) = *p++;

  free(temp);
  return vdst;
}

// this is wrong, it does not make any temp allocation
/*
void*
memmove(void *vdst, const void *vsrc, int n)
{
  char *dst;
  const char *src;

  dst = vdst;
  src = vsrc;

  while (n-- > 0)
    *dst++ = *src++;

  return vdst;
}
*/

// extra
char *strncpy(char *s, const char *t, int n) {
  char *os = s;
  while (n-- > 0 && (*s++ = *t++) != 0)
    ;

  return os;
}

int strncmp(const char *p, const char *q, int n) {
  if (n == 0)
    return 0;

  while (--n > 0 && *p && *p == *q)
    p++, q++;

  return (uchar)*p - (uchar)*q;
}

char *strcat(char *s, const char *t) {
  char *os = s;
  s = s + strlen(s);

  while (*t != 0)
    *s++ = *t++;

  *s = 0;
  return os;
}

char *strncat(char *s, const char *t, int n) {
  if (n == 0)
    return 0;

  char *os = s;
  int s_len = strlen(s);

  s = s + s_len;
  n -= s_len;

  while (n-- > 0 && *t != 0)
    *s++ = *t++;

  return os;
}

// taken from printf.c: printint
char *itoa(int i, char *a) {
  static char digits[] = "0123456789abcdef";
  char buf[16], *b = a;

  int j, neg, base = 10;

  uint x = (neg = i < 0) ? -i : i;
  j = 0;

  do {
    buf[j++] = digits[x % base];
  } while ((x /= base) != 0);

  if (neg)
    buf[j++] = '-';

  while (--j >= 0)
    *b++ = buf[j];

  *b = 0;
  return a;
}

char getc(void) {
  char c;
  read(0, &c, 1);

  return c;
}

char *getline(char *buf, int max) {
  gets(buf, max, '\n');
  return buf;
}

// for pseudo random number generator
static unsigned long long __randomseed = (unsigned long long)0xdeadbeefcafebabe,
                          __randomstate =
                              (unsigned long long)0xdeadbeefcafebabe;

uint random(void) {
  char newbit;
  int i = 0;

  // this should be pseudo-random enough
  // at least I cannot calculate all of this by myself :p

  for (; i < 16; i++) {
    newbit = (__randomstate ^ (__randomstate >> 1) ^ (__randomstate >> 3) ^
              (__randomstate >> 7)) &
             1;
    __randomstate = (__randomstate >> 1) | ((unsigned long long)newbit << 63);
  }

  for (; i < 48; i++) {
    newbit = (__randomstate ^ (__randomstate >> 17) ^ (__randomstate >> 29) ^
              (__randomstate >> 43)) &
             1;
    __randomstate = (__randomstate >> 1) | ((unsigned long long)newbit << 63);
  }

  for (; i < 64; i++) {
    newbit = (__randomstate ^ (__randomstate >> 53) ^ (__randomstate >> 59) ^
              (__randomstate >> 61)) &
             1;
    __randomstate = (__randomstate >> 1) | ((unsigned long long)newbit << 63);
  }

  return __randomstate & 0xffffffff;
}

void seedrandom(uint seed) {
  __randomseed = seed;
  __randomstate = __randomseed ^ (unsigned long long)0xdeadbeefcafebabe;
}
