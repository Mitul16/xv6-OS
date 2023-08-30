#include "signal.h"
#include "defs.h"
#include "spinlock.h"
#include "types.h"

// use a queue to store more than one signal for each CPU (upto a MAXSIG limit)
// and replay them in their order of arrival

typedef int (*sighandler_t)(void);

extern struct proc *initproc;

static struct spinlock siglock;

// working on a process for now instead of changing the flow of control
// by saving the `context` and pushing signal handler and `sigret` on stack
static int sigkill(struct proc *);
static int sigstop(struct proc *);

static int (*sighandlers[])(struct proc *) = {
    [SIGKILL] sigkill,
    [SIGSTOP] sigstop,
};

void siginit(void) { initlock(&siglock, "signal"); }

void signal_handler(struct cpu *c) {
  // only SIGKILL is supported for now, others would require building
  // a signal frame

  acquire(&siglock);
  if (!c->signum) {
    release(&siglock);
    return;
  }

  if (c->lastproc && c->lastproc != initproc) {
    cprintf("\n%s: ", RED("signal_handler"));

    if (c->signum >= 0 && c->signum < NELEM(sighandlers) &&
        sighandlers[c->signum]) {
      sighandlers[c->signum](c->lastproc);
    } else {
      cprintf("invalid signum: %d\n", c->signum);
    }
  }

  c->signum = 0;
  release(&siglock);
}

void sigsend(uint signum) {
  acquire(&siglock);
  mycpu()->signum = signum;
  release(&siglock);
}

void sigret(void) { cprintf("sigret\n"); }

static int sigkill(struct proc *p) {
  if (!p->killed) {
    p->killed = 1;
    cprintf("(pid: %d) process killed!\n", p->pid);
    return 0;
  }

  cprintf("(pid: %d) process is already killed!\n", p->pid);
  return -1;
}

static int sigstop(struct proc *p) {
  // we need some context to `fg` it
  // `kill -CONT $PID` or `kill -s SIGCONT $PID`
  cprintf("SIGSTOP is not implemented\n");
  return -1;

  /*
    // ptable.lock
    // detach from the parent and reparent to `initproc`
    p->state = SLEEPING;
    p->parent = initproc;
    p->parent->state = RUNNABLE;
    cprintf("(pid: %d) process suspended!\n", p->pid);
    return 0;
  */

  //  cprintf("(pid: %d) process is already suspended!\n", p->pid);
  //  return -1;
}
