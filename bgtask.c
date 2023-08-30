#include "time.h"
#include "user.h"

#define SLEEPMILLIS 4000

int main(int argc, char *argv[]) {
  bprintf(1, "bgtask started - time interval = %d milliseconds\n", SLEEPMILLIS);

  int pid;

  if ((pid = fork()) < 0) {
    bprintf(2, "error: fork failed!\n");
    exit();
  }

  if (pid == 0) {
    for (;;) {
      bprintf(1, "bgtask: cpu = %d, pid = %d, uptime = %d\n", getcpuid(),
              getpid(), uptime());
      pause2(SLEEPMILLIS);
    }
  }

  exit();
}
