#include "user.h"

int main(int argc, char *argv[]) {
  for (;;) {
    bprintf(1, "loop3: cpu = %d, uptime = %d\n", getcpuid(), uptime());
  }

  exit();
}
