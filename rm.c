#include "user.h"

int main(int argc, char *argv[]) {
  // add dir not empty, check isdirempty
  // or add flags or `rmdir`

  if (argc < 2)
    bprintf(2, "usage: rm <file or directory> [...]\n");
  else
    for (int i = 1; i < argc; i++) {
      if (unlink(argv[i]) < 0) {
        bprintf(2, "rm: failed to delete %s\n", argv[i]);
      }
    }

  exit();
}
