#include "fs.h"
#include "user.h"

int main(int argc, char *argv[]) {
  int i;

  if (argc < 2) {
    bprintf(2, "usage: mkdir <directory> [...]\n");
    exit();
  }

  for (i = 1; i < argc; i++) {
    if (strlen(argv[i]) > DIRSIZ)
      bprintf(2, "error: name cannot exceed %d characters\n", DIRSIZ);
    if (mkdir(argv[i]) < 0) {
      bprintf(2, "error: failed to create %s\n", argv[i]);
      break;
    }
  }

  exit();
}
