#include "user.h"

int main(int argc, char *argv[]) {
  int i, addnewline = 1, argstart = 1;

  if (strcmp(argv[1], "-n") == 0) {
    addnewline = 0;
    argstart++;
  }

  for (i = argstart; i < argc; i++)
    bprintf(1, "%s%s", argv[i], i + 1 < argc ? " " : (addnewline ? "\n" : ""));

  exit();
}
