#include "syscall.h"
#include "traps.h"

#define SYSCALL(name) \
  .globl name; \
  name: \
    movl $SYS_ ## name, %eax; \
    int $T_SYSCALL; \
    ret

SYSCALL(fork)
SYSCALL(exit)
SYSCALL(wait)
SYSCALL(pipe)
SYSCALL(read)
SYSCALL(write)
SYSCALL(close)
SYSCALL(kill)
SYSCALL(exec)
SYSCALL(open)
SYSCALL(mknod)
SYSCALL(unlink)
SYSCALL(fstat)
SYSCALL(link)
SYSCALL(mkdir)
SYSCALL(chdir)
SYSCALL(dup)
SYSCALL(getpid)
SYSCALL(sbrk)
SYSCALL(sleep)
SYSCALL(uptime)

// extra
SYSCALL(getprocinfo)
SYSCALL(cmostime)
SYSCALL(halt)
SYSCALL(getcwd)
SYSCALL(getcpuid)
SYSCALL(pipepoll)
SYSCALL(getppid)
SYSCALL(is_pipe)
SYSCALL(clrscr)
SYSCALL(getcattr)
SYSCALL(setcattr)
SYSCALL(cflush)
SYSCALL(getuname)
SYSCALL(setuname)

// lab assignment 5
SYSCALL(wait2)

// mid-term project
SYSCALL(stat)
SYSCALL(opendir)
SYSCALL(readdir)
SYSCALL(closedir)
SYSCALL(symlink)
SYSCALL(readlink)

// extra
SYSCALL(setbgcolor)
SYSCALL(setfgcolor)
