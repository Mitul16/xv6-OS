#include "time.h"
#include "user.h"

void showusage() {
  bprintf(
      2,
      "usage: sleep2 <time in seconds or milliseconds> (default: seconds)\n");
  bprintf(2, "examples:\n\tsleep2 1\n\tsleep2 2s\n\tsleep2 100ms\n");
  exit();
}

int main(int argc, char *argv[]) {
  if (argc != 2)
    showusage();

  int millis, multiplier = 1000, i = 0;

  while (argv[1][i]) {
    if (argv[1][i] < '0' || argv[1][i] > '9')
      break;

    i++;
  }

  if (strcmp(argv[1] + i, "s") == 0)
    multiplier = 1000;
  else if (strcmp(argv[1] + i, "ms") == 0)
    multiplier = 1;
  else if (argv[1][i] != '\0')
    showusage();

  if ((millis = atoi(argv[1])) <= 0)
    showusage();

  millis *= multiplier;
  pause2(millis);
  exit();
}
