#include "fcntl.h"
#include "stat.h"
#include "user.h"

#define BUFSIZE 4096

void showusage() {
  bprintf(2, "usage: cp src-file dest-file\n");
  exit();
}

void show_notfileerror(char *name) {
  bprintf(2, "cp: %s is not a file\n", name);
  showusage();
}

int main(int argc, char *argv[]) {
  int fd_src, fd_dest;
  struct stat st;

  if (argc != 3)
    showusage();

  if ((fd_src = open(argv[1], O_RDONLY)) < 0) {
    bprintf(2, "cp: error while opening `%s`\n", argv[1]);
    exit();
  }

  if (fstat(fd_src, &st) < 0) {
    bprintf(2, "cp: cannot stat %s\n", argv[1]);
    exit();
  }

  if (st.type != T_FILE)
    show_notfileerror(argv[1]);

  // if dest is a dir, like `cp /path/to/file .`
  // implement it as well

  if ((fd_dest = open(argv[2], O_WRONLY | O_CREATE)) < 0) {
    bprintf(2, "cp: error while opening `%s`\n", argv[2]);
    exit();
  }

  if (fstat(fd_dest, &st) < 0) {
    bprintf(2, "cp: cannot stat %d\n", argv[2]);
    exit();
  }

  if (st.type != T_FILE)
    show_notfileerror(argv[2]);

  int recv;
  char *buf = (char *)malloc(BUFSIZE);

  if (!buf) {
    bprintf(2, "cp: error while allocating buffer\n");
    close(fd_src);
    close(fd_dest);
    exit();
  }

  while ((recv = read(fd_src, buf, BUFSIZE)) > 0) {
    write(fd_dest, buf, recv);
  }

  free(buf);

  close(fd_src);
  close(fd_dest);

  exit();
}
