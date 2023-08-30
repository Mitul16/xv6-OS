#include "time.h"
#include "user.h"

#define TALKTIME 10
#define POLLMILLIS 200
#define MAXPOLL 6

int main(void) {
  bprintf(1, "Welcome to pctalks2!\n");

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
  int recv, time = 0, poll, num_poll;
  struct rtcdate start;
  cmostime(&start);

  if (pid == 0) {
    close(pipefd_child[1]);
    close(pipefd_parent[0]);

    while (time < TALKTIME) {
      num_poll = 0;
      while (num_poll < MAXPOLL && (poll = pipepoll(pipefd_child[0])) == 0 &&
             timespent(start) < TALKTIME) {
        // bprintf(2, "child: polling...\n");
        pause2(POLLMILLIS);
        num_poll++;
      }

      if (!(timespent(start) < TALKTIME))
        break;

      if (poll < 0) {
        bprintf(2, "child: pipe closed!\n");
        break;
      }

      if (num_poll == MAXPOLL) {
        bprintf(2, "child: parent timed out\n");
        break;
      }

      bprintf(1, "child> pipe> ");
      while ((recv = read(pipefd_child[0], buf, sizeof(buf))) > 0) {
        write(1, buf, recv);
        if (buf[recv] == 0)
          break;
      }

      bprintf(pipefd_parent[1], "from: %s, pid: %d, cpu: %d, uptime: %d\n",
              "child", getpid(), getcpuid(), uptime());

      time++;
      while (timespent(start) < time)
        pause2(100);

      if (time >= TALKTIME / 2) {
        bprintf(1, "child: exiting early!\n");
        break;
      }
    }

    if (!(timespent(start) < TALKTIME))
      bprintf(1, "child: timeout\n");

    close(pipefd_child[0]);
    close(pipefd_parent[1]);
  } else {
    close(pipefd_parent[1]);
    close(pipefd_child[0]);

    while (time < TALKTIME) {
      bprintf(pipefd_child[1], "from: %s, pid: %d, cpu: %d, uptime: %d\n",
              "parent", getpid(), getcpuid(), uptime());

      num_poll = 0;
      while (num_poll < MAXPOLL && (poll = pipepoll(pipefd_parent[0])) == 0 &&
             timespent(start) < TALKTIME) {
        // bprintf(2, "parent: polling...\n");
        pause2(POLLMILLIS);
        num_poll++;
      }

      if (!(timespent(start) < TALKTIME))
        break;

      if (poll < 0) {
        bprintf(2, "parent: pipe closed!\n");
        break;
      }

      if (num_poll == MAXPOLL) {
        bprintf(2, "parent: child timed out\n");
        break;
      }

      bprintf(1, "parent> pipe> ");
      while ((recv = read(pipefd_parent[0], buf, sizeof(buf))) > 0) {
        write(1, buf, recv);
        if (buf[recv] == 0)
          break;
      }

      time++;
      while (timespent(start) < time)
        pause2(100);
    }

    if (!(timespent(start) < TALKTIME))
      bprintf(1, "parent: timeout\n");

    close(pipefd_parent[0]);
    close(pipefd_child[1]);

    while ((wpid = wait()) >= 0 && wpid != pid)
      ;
  }

  bprintf(1, "%s: exiting\n", (pid ? "parent" : "child"));
  exit();
}
