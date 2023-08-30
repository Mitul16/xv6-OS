#include "fcntl.h"
#include "stat.h"
#include "user.h"

void showusage() {
  bprintf(2, "usage: mv src-file dest-file\n");
  exit();
}

int main(int argc, char *argv[]) {
  struct stat st_src, st_dest;

  if (argc != 3)
    showusage();

  if (stat(argv[1], S_NOFOLLOW, &st_src) < 0) {
    bprintf(2, "mv: cannot stat %s\n", argv[1]);
    exit();
  }

  if (stat(argv[2], S_NOFOLLOW, &st_dest) >= 0) {
    bprintf(2, "mv: %s already exists\n", argv[2]);
    exit();
  }

  if (link(argv[1], argv[2]) < 0) {
    bprintf(2, "mv: link failed\n");
    exit();
  }

  if (unlink(argv[1]) < 0) {
    bprintf(2, "mv: unlink failed\n");
    exit();
  }

  exit();
}
