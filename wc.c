#include "stat.h"
#include "user.h"

char buf[512];

void wc(int fd, char *name) {
  int i, n;
  int l, w, c, inword;

  l = w = c = 0;
  inword = 0;

  while ((n = read(fd, buf, sizeof(buf))) > 0) {
    for (i = 0; i < n; i++) {
      c++;

      if (buf[i] == '\n')
        l++;

      if (strchr(" \r\t\n\v", buf[i]))
        inword = 0;
      else if (!inword) {
        w++;
        inword = 1;
      }
    }
  }

  if (n < 0) {
    bprintf(2, "wc: read error\n");
    exit();
  }

  bprintf(1, "%d %d %d %s\n", l, w, c, name);
}

int main(int argc, char *argv[]) {
  int fd, i;
  struct stat st;

  if (argc <= 1) {
    wc(0, "");
    exit();
  }

  for (i = 1; i < argc; i++) {
    if ((fd = open(argv[i], 0)) < 0) {
      bprintf(2, "wc: cannot open %s\n", argv[i]);
      continue;
    }

    if (fstat(fd, &st) < 0) {
      bprintf(2, "wc: cannot stat %s\n", argv[i]);
      close(fd);
      continue;
    }

    if (st.type != T_FILE) {
      bprintf(2, "wc: %s is not a file\n", argv[i]);
      close(fd);
      continue;
    }

    wc(fd, argv[i]);
    close(fd);
  }

  exit();
}
