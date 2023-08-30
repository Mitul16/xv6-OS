#include "fcntl.h"
#include "proc.h"
#include "time.h"
#include "user.h"

int main(int argc, char *argv[]) {
  int pid;
  struct ptime pt;

  char tempfile[] = "wait2.tempfile";

  if ((pid = fork()) < 0) {
    bprintf(2, "fork failed!\n");
    exit();
  }

  if (pid == 0) {
    int fd;
    char temp;

    int iter = 4, N = 256;
    char message[] = "this will be written to the file and read from it.\n";

    for (int i = 0; i < iter; i++) {
      bprintf(1, "iteration: %d\n", i + 1);
      bprintf(1, "child: opening %s...\n", tempfile);

      if ((fd = open(tempfile, O_CREATE | O_RDWR)) < 0) {
        bprintf(2, "child: error while opening %s!\n", tempfile);
        exit();
      }

      bprintf(1, "child: writing to %s...\n", tempfile);

      for (int j = 0; j < N; j++)
        write(fd, message, sizeof(message));

      bprintf(1, "child: closing %s...\n", tempfile);
      close(fd);

      putc(1, '\n');
      bprintf(1, "child: opening %s...\n", tempfile);

      if ((fd = open(tempfile, O_CREATE | O_RDWR)) < 0) {
        bprintf(2, "child: error while opening %s!\n", tempfile);
        exit();
      }

      bprintf(1, "child: reading from %s...\n", tempfile);
      while (read(fd, &temp, 1) > 0)
        ;

      bprintf(1, "child: closing %s...\n", tempfile);
      close(fd);

      putc(1, '\n');
      bprintf(1, "child: removing %s...\n", tempfile);

      if (unlink(tempfile) < 0) {
        bprintf(2, "child: error while removing %s!\n", tempfile);
        exit();
      }

      putc(1, '\n');
    }
  } else {
    wait2(&pt);
    bprintf(1, "ctime: %d\nttime: %d\nretime: %d\nrutime: %d\nstime: %d\n",
            pt.ctime, pt.ttime, pt.retime, pt.rutime, pt.stime);
  }

  exit();
}
