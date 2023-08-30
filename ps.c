#include "param.h"
#include "proc.h"
#include "user.h"

static struct uproc uproc_list[NPROC];
static char temp[44];

// extra
char *ljust(char *dest, const char *src, const char fillchar, int len) {
  char *d = dest;

  while (len > 0 && *src) {
    *dest++ = *src++;
    len--;
  }

  while (len > 0) {
    *dest++ = fillchar;
    len--;
  }

  *dest = 0;
  return d;
}

int main(int argc, char *argv[]) {
  // there is some issue if we run this without fork
  // possibly because current process state is `sleep`
  // or we couldn't allocate memory
  // we get some trap error 14, page fault if run without `fork`

  // using `static struct uproc ...` resolved this issue, it was perhaps caused
  // due to memory allocations

  int proccount = getprocinfo((void *)uproc_list), len;

  bprintf(1,
          "PID\tNAME\t\tCWD\t\t\t\t\tSTATE\tPPID\tPNAME\t\tSIZE\tOPEN fd(s)\n");

  if (proccount > 0) {
    for (struct uproc *up = uproc_list; up < &uproc_list[proccount]; up++) {
      strcpy(up->name, ljust(temp, up->name, ' ', 16));
      strcpy(up->state, ljust(temp, up->state, ' ', 8));
      strcpy(up->pname, ljust(temp, up->pname, ' ', 16));

      if ((len = strlen(up->cwd)) >= 40) {
        strncpy(up->cwd, "...", 3);

        for (int i = 3; i < 40; i++)
          up->cwd[i] = up->cwd[len - 39 + i];
      } else {
        strcpy(up->cwd, ljust(temp, up->cwd, ' ', 40));
      }

      up->cwd[39] = 0;

      bprintf(1, "%s\t%s%s %s%s\t%s%d\t%d\n", itoa(up->pid, temp), up->name,
              up->cwd, up->state, up->ppid > 0 ? itoa(up->ppid, temp) : "???",
              up->pname, up->size, up->open_fds);
    }
  } else {
    bprintf(2, "ps: no info received from 'getprocinfo'\n");
  }

  exit();
}
