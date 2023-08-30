#include "time.h"
#include "user.h"

void showusage() { bprintf(2, "usage: sleep <time in milliseconds>\n"); }

int main(int argc, char *argv[]) {
  // some issue with this program
  // something with `proc->state`, the process exits immediately
  // if we use `printf(..something..)` then it works
  // perhaps it puts the process to RUNNING state

  // proc.c - wakeup1(), there is an if block for the wait() call
  // it checks if (proc->state == SLEEPING)

  if (argc != 2) {
    showusage();
  } else {
    int millis;
    if ((millis = atoi(argv[1])) > 0) {
      // do something to be become RUNNING
      // printf(2, "sleeping for %d second%s\n", sleep_time, sleep_time > 1 ?
      // "s": "");
      pause(millis);
    } else {
      showusage();
    }
  }

  bprintf(2, "sleep: if this didn't work as expected\n\
see (proc->state == SLEEPING) in proc.c, for wait() call\n");
  exit();
}
