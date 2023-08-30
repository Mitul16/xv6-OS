#include "time.h"
#include "user.h"

int main(int argc, char *argv[]) {
  seedrandom(uptime());

  for (;;) {
    bprintf(1, "random: %d\n", random() & 0xffff);
    pause2(40);
  }

  exit();
}
