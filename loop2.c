#include "time.h"
#include "user.h"

int main(int argc, char *argv[]) {
  for (;;) {
    void *ptr = malloc(4096);
    pause(1);
    free(ptr);
  }
  exit();
}
