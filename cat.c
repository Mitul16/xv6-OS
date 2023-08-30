#include "stat.h"
#include "user.h"

void cat(int fd) {
  int n;
  char buf[512];

  while ((n = read(fd, buf, sizeof(buf))) > 0) {
    if (write(1, buf, n) != n) {
      bprintf(1, "cat: write error\n");
      exit();
    }
  }

  if (n < 0) {
    bprintf(1, "cat: read error\n");
    exit();
  }
}

int main(int argc, char *argv[]) {
  int fd, i;
  struct stat st;

  if (argc <= 1 || (argc == 2 && strcmp(argv[1], "-") == 0)) {
    cat(0);
    exit();
  }

  for (i = 1; i < argc; i++) {
    if ((fd = open(argv[i], 0)) < 0) {
      bprintf(1, "cat: cannot open %s\n", argv[i]);
      continue;
    }

    if (fstat(fd, &st) < 0) {
      bprintf(2, "cat: cannot stat %s\n", argv[i]);
      close(fd);
      continue;
    }

    if (st.type != T_FILE) {
      bprintf(2, "cat: %s is not a file\n", argv[i]);
      close(fd);
      continue;
    }

    cat(fd);
    close(fd);
  }

  exit();
}
