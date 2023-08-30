// Create an orphan process that
// will be reparented (to init) at exit()

#include "time.h"
#include "user.h"

#define WAIT_TIME 2

int main(void) {
  int pid;

  if ((pid = fork()) < 0) {
    bprintf(1, "error: fork failed\n");
    exit();
  }

  if (pid == 0) {
    bprintf(1, "wait for %d seconds\n", WAIT_TIME);
    pause2(WAIT_TIME * 1000); // Let parent exit before child.
  }

  exit();
}
