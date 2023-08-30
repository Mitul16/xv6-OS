#include "time.h"
#include "user.h"

#define TALKTIME 10

int main(void) {
  bprintf(1, "Welcome to pctalks!\n");

  int pid, wpid;
  int pipefd_parent[2], pipefd_child[2];

  if (pipe(pipefd_parent) < 0 || pipe(pipefd_child)) {
    bprintf(2, "error: couldn't create pipe\n");
    exit();
  }

  bprintf(1, "pipefd_parent: r:%d, w:%d\n", pipefd_parent[0], pipefd_parent[1]);
  bprintf(1, "pipefd_child : r:%d, w:%d\n", pipefd_child[0], pipefd_child[1]);

  putc(1, '\n');

  if ((pid = fork()) < 0) {
    bprintf(2, "error: fork failed\n");
    exit();
  }

  char buf[64];
  int recv, time = 0, sayname;
  struct rtcdate start;
  cmostime(&start);

  if (pid == 0) {
    close(pipefd_child[1]);
    close(pipefd_parent[0]);

    while (time < TALKTIME) {
      sayname = 1;
      while ((recv = read(pipefd_child[0], buf, sizeof(buf))) > 0) {
        bprintf(1, "%s%s", (sayname ? "child> pipe> " : ""), buf);
        if (buf[recv] == 0)
          break;
        sayname = 0;
      }

      if (recv <= 0) {
        bprintf(2, "child: pipe closed!\n");
        break;
      }

      bprintf(pipefd_parent[1], "from: %s, pid: %d, cpu: %d, uptime: %d\n",
              "child", getpid(), getcpuid(), uptime());

      time++;
      while (timespent(start) < time)
        sleep(millisToticks(100));

      if (time >= TALKTIME / 2) {
        bprintf(1, "child: exiting early!\n");
        break;
      }
    }

    close(pipefd_child[0]);
    close(pipefd_parent[1]);

    if (!(time < TALKTIME))
      bprintf(1, "child: timeout\n");
  } else {
    close(pipefd_parent[1]);
    close(pipefd_child[0]);

    while (time < TALKTIME) {
      bprintf(pipefd_child[1], "from: %s, pid: %d, cpu: %d, uptime: %d\n",
              "parent", getpid(), getcpuid(), uptime());

      sayname = 1;
      while ((recv = read(pipefd_parent[0], buf, sizeof(buf))) > 0) {
        bprintf(1, "%s%s", (sayname ? "parent> pipe> " : ""), buf);
        if (buf[recv] == 0)
          break;
        sayname = 0;
      }

      if (recv <= 0) {
        bprintf(2, "parent: pipe closed!\n");
        break;
      }

      time++;
      while (timespent(start) < time)
        sleep(millisToticks(100));
    }

    close(pipefd_parent[0]);
    close(pipefd_child[1]);

    if (!(time < TALKTIME))
      bprintf(1, "parent: timeout\n");

    while ((wpid = wait()) >= 0 && wpid != pid)
      ;
  }

  bprintf(1, "%s: exiting\n", (pid ? "parent" : "child"));
  exit();
}
