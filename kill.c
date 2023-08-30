#include "user.h"

int main(int argc, char **argv) {
  int pid, result;

  if (argc < 2)
    bprintf(2, "usage: kill <pid> [...]\n");
  else {
    for (int i = 1; i < argc; i++) {
      if ((pid = atoi(argv[i])) <= 0) {
        bprintf(2, "error: invalid pid %s\n", argv[i]);
        continue;
      }

      result = kill(pid);

      if (result == 0) // success
        ;
      else if (result == -1) // failed
        bprintf(2, "error: invalid pid %d\n", pid);
      else if (result == -2) // not permitted
        bprintf(2, "error: cannot kill pid %d\n", pid);
    }
  }

  exit();
}
