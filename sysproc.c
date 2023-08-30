#include "date.h"
#include "defs.h"
#include "memlayout.h"
#include "mmu.h"
#include "param.h"
#include "proc.h"
#include "time.h"
#include "types.h"
#include "x86.h"

extern struct spinlock tickslock;
extern uint ticks;

static char uname[32];

int sys_fork(void) { return fork(); }

int sys_exit(void) {
  exit();
  return 0; // not reached
}

int sys_wait(void) { return wait(); }

int sys_kill(void) {
  int pid;

  if (argint(0, &pid) < 0)
    return -1;

  return kill(pid);
}

int sys_getpid(void) { return myproc()->pid; }

// what is this doing?
// shouldn't it return char*?

int sys_sbrk(void) {
  int addr;
  uint n;

  if (argint(0, (int *)&n) < 0)
    return -1;

  addr = myproc()->sz;
  if (growproc(n) < 0)
    return -1;

  return addr;
}

int sys_sleep(void) {
  uint nticks;
  uint ticks0;

  if (argint(0, (int *)&nticks) < 0)
    return -1;

  acquire(&tickslock);
  ticks0 = ticks;

  while (ticks - ticks0 <= nticks) {
    if (myproc()->killed) {
      release(&tickslock);
      return -1;
    }

    sleep(&ticks, &tickslock);
  }

  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int sys_uptime(void) {
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

// extra
int sys_getprocinfo(void) {
  void *uproc_list;

  if (argptr(0, (char **)&uproc_list, sizeof(void *)) < 0)
    return -1;

  return getprocinfo(uproc_list);
}

int sys_cmostime(void) {
  struct rtcdate *rtc;

  if (argptr(0, (char **)&rtc, sizeof(rtc)) < 0)
    return -1;

  cmostime(rtc);
  return 0;
}

int sys_halt(void) {
  // this is not working :/
  cprintf("halting...\n");
  outw(0xb004, 0x2000);
  return 0;
}

int sys_getcwd(void) {
  char *cwd;

  if (argptr(0, &cwd, sizeof(cwd)) < 0)
    return -1;

  getcwd(cwd);
  return 0;
}

int sys_getcpuid(void) { return myproc()->cpuid; }

int sys_getppid(void) {
  return (myproc()->parent ? myproc()->parent->pid : -1);
}

int sys_clrscr(void) {
  clrscr();
  return 0;
}

int sys_getcattr(void) {
  int *echo, *intr;

  if (argptr(0, (char **)&echo, sizeof(echo)) < 0 ||
      argptr(1, (char **)&intr, sizeof(intr)) < 0)
    return -1;

  getcattr(echo, intr);
  return 0;
}

int sys_setcattr(void) {
  int echo, intr;

  if (argint(0, &echo) < 0 || argint(1, &intr) < 0)
    return -1;

  setcattr(echo, intr);
  return 0;
}

int sys_cflush(void) {
  flushstdin();
  return 0;
}

int sys_getuname(void) {
  char *name;

  if (argstr(0, &name) < 0)
    return -1;

  safestrcpy(name, uname, sizeof(uname));
  return 0;
}

int sys_setuname(void) {
  char *name;

  if (argstr(0, &name) < 0)
    return -1;

  safestrcpy(uname, name, sizeof(uname));
  return 0;
}

// lab assignment 5
int sys_wait2(void) {
  struct ptime *pt;

  if (argptr(0, (char **)&pt, sizeof(pt)) < 0)
    return -1;

  return wait2(pt);
}
