#include "user.h"

int main(int argc, char *argv[]) {
  int create_symlink = 0;
  int argc_old = -1, argc_new = -1;

  if (argc != 3 && argc != 4) {
    bprintf(2, "usage: ln [-s] old new\n");
    exit();
  }

  // parse the option arguments
  for (int i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      char *arg = argv[i] + 1;

      while (*arg != '\0') {
        switch (*arg) {
        case 's':
          create_symlink = 1;
          break;

        default:
          bprintf(2, "Invalid argument: %c in %s\n", *arg, argv[i]);
          exit();
        }

        arg++;
      }
    }
  }

  for (int i = 1; i < argc; i++) {
    if (argv[i][0] != '-') {
      if (argc_old < 0) {
        argc_old = i;
        continue;
      }

      argc_new = i;
    }
  }

  if (create_symlink) {
    if (symlink(argv[argc_old], argv[argc_new]) < 0)
      bprintf(2, "link -s %s %s: failed\n", argv[argc_old], argv[argc_new]);
  } else {
    if (link(argv[argc_old], argv[argc_new]) < 0)
      bprintf(2, "link %s %s: failed\n", argv[argc_old], argv[argc_new]);
  }

  exit();
}
