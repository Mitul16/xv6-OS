#include "proc.h"
#include "defs.h"
#include "memlayout.h"
#include "mmu.h"
#include "param.h"
#include "spinlock.h"
#include "types.h"
#include "x86.h"

static struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

struct proc *initproc;
static int nextpid = 1;

// extra
void _initial_forkret(void);
static void (*_forkret_ptr)(void) = _initial_forkret;

void forkret(void);
extern void trapret(void);
static void wakeup1(void *);

static char *procstate_names[] = {
    [UNUSED] "unused",   [EMBRYO] "embryo",  [SLEEPING] "sleep ",
    [RUNNABLE] "runble", [RUNNING] "run   ", [ZOMBIE] "zombie"};

void pinit(void) { initlock(&ptable.lock, "ptable"); }

// Must be called with interrupts disabled
int cpuid() { return mycpu() - cpus; }

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu *mycpu(void) {
  int apicid, i;

  if (readeflags() & FL_IF)
    panic("mycpu", "interrupts enabled");

  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].

  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }

  panic("mycpu", "unknown apicid");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc *myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc *allocproc(void) {
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if (p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  // lab assignment 5
  // save creation time and set other values
  p->time.ctime = sys_uptime();
  p->time.retime = p->time.rutime = p->time.stime = 0;
  p->time.ttime = -1;

  p->state = EMBRYO;
  p->pid = nextpid++;

  release(&ptable.lock);

  // Allocate kernel stack.
  if ((p->kstack = kalloc()) == 0) {
    p->state = UNUSED;
    return 0;
  }

  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof(*p->tf);
  p->tf = (struct trapframe *)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint *)sp = (uint)trapret;

  sp -= sizeof(*p->context);
  p->context = (struct context *)sp;
  memset(p->context, 0, sizeof(*p->context));
  p->context->eip = (uint)_forkret_ptr;

  return p;
}

// Set up first user process.
void userinit(void) {
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();

  initproc = p;
  if ((p->pgdir = setupkvm()) == 0)
    panic("userinit", "out of memory?");

  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;

  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0; // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/", 1);
  safestrcpy(p->cwd_str, "/", sizeof(p->cwd_str));

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);
  p->state = RUNNABLE;
  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int growproc(int n) {
  struct proc *curproc = myproc();
  uint sz = curproc->sz;

  if (n > 0) {
    if ((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if (n < 0) {
    if ((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }

  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int fork(void) {
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if ((np = allocproc()) == 0)
    return -1;

  // Copy process state from proc.
  if ((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0) {
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }

  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for (i = 0; i < NOFILE; i++)
    if (curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);

  np->cwd = idup(curproc->cwd);
  safestrcpy(np->cwd_str, curproc->cwd_str, sizeof(curproc->cwd_str));
  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);
  np->state = RUNNABLE;
  release(&ptable.lock);
  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void exit(void) {
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if (curproc == initproc)
    panic("exit", "init exiting");

  // lab assignment 5
  // save termination time
  curproc->time.ttime = sys_uptime();

  // Close all open files.
  for (fd = 0; fd < NOFILE; fd++) {
    if (curproc->ofile[fd]) {
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;
  safestrcpy(curproc->cwd_str, "", sizeof(curproc->cwd_str));

  acquire(&ptable.lock);

  // do not wake up parent if children have children?
  // int havegrandkids = 0;

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
    if (p->parent == curproc) {
      p->parent = initproc;

      if (p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;

  sched();
  panic("exit", "zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int wait(void) {
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();

  acquire(&ptable.lock);
  for (;;) {
    // Scan through table looking for exited children.
    havekids = 0;
    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
      if (p->parent != curproc)
        continue;

      havekids = 1;
      if (p->state == ZOMBIE) {
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if (!havekids || curproc->killed) {
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock); // DOC: wait-sleep
  }
}

// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void scheduler(void) {
  struct proc *p;
  struct cpu *c = mycpu();
  const int cpuid = c - cpus;
  c->proc = 0;
  c->signum = 0;

  for (;;) {
    // cprintf("scheduler: cpu = %d, uptime = %d\n", cpuid, sys_uptime());

    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
      if (p->state != RUNNABLE)
        continue;

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      c->proc = p;
      switchuvm(p);
      p->state = RUNNING;
      p->cpuid = cpuid;

      swtch(&(c->scheduler), p->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->lastproc = c->proc;
      c->proc = 0;
      p->cpuid = -1;
    }
    release(&ptable.lock);
  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void sched(void) {
  int intena;
  struct proc *curproc = myproc();

  if (!holding(&ptable.lock))
    panic("sched", "ptable.lock is free");

  if (mycpu()->ncli != 1)
    panic("sched", "locks");

  if (curproc->state == RUNNING)
    panic("sched", "running");

  if (readeflags() & FL_IF)
    panic("sched", "interruptible");

  intena = mycpu()->intena;
  swtch(&curproc->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void yield(void) {
  acquire(&ptable.lock); // DOC: yieldlock
  myproc()->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void _initial_forkret(void) {
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  // Some initialization functions must be run in the context
  // of a regular process (e.g., they call sleep), and thus cannot
  // be run from main().
  iinit(ROOTDEV);
  initlog(ROOTDEV);

  // update _forkret_ptr
  // now that we have initialized required stuff
  // there is no need to come to this function again
  _forkret_ptr = forkret;

  // Return to "caller", actually trapret (see allocproc).
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void forkret(void) {
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void sleep(void *chan, struct spinlock *lk) {
  struct proc *p = myproc();

  if (p == 0)
    panic("sleep", "no process");

  if (lk == 0)
    panic("sleep", "no lock");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if (lk != &ptable.lock) { // DOC: sleeplock0
    acquire(&ptable.lock);  // DOC: sleeplock1
    release(lk);
  }

  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if (lk != &ptable.lock) { // DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void wakeup1(void *chan) {
  struct proc *p;

  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if (p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
void wakeup(void *chan) {
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int kill(int pid) {
  // prevent user from killing kernel process(es)
  // `kill 1` (would kill `init`)
  if (pid == initproc->pid) {
    return -2; // operation not allowed
  }

  struct proc *p;

  acquire(&ptable.lock);
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
    if (p->pid == pid) {
      p->killed = 1;

      // Wake process from sleep if necessary.
      if (p->state == SLEEPING)
        p->state = RUNNABLE;

      release(&ptable.lock);
      return 0; // successful
    }
  }

  release(&ptable.lock);
  return -1; // it was an invalid pid
}

// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void procdump(void) {
  int i;
  struct proc *p;
  uint pc[10];

  cprintf("--- procdump ---\n");
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
    if (p->state == UNUSED)
      continue;

    cprintf("%d %s %s", p->pid,
            (p->state >= 0 && p->state < NELEM(procstate_names))
                ? procstate_names[p->state]
                : "???",
            p->name);

    if (p->state == SLEEPING) {
      getcallerpcs((uint *)p->context->ebp + 2, pc);

      for (i = 0; i < 10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }

    cprintf("\n");
  }
}

// extra
int getprocinfo(void *ptr) {
  struct uproc *uproc_list = (struct uproc *)ptr;

  if (!uproc_list) {
    cprintf("getprocinfo: received nullptr\n");
    return 0;
  }

  // pid, name, cwd, state, pname, ppid, number of open fds
  struct proc *p;
  struct uproc up;

  int count = 0;

  acquire(&ptable.lock);
  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
    if (p->state == UNUSED)
      continue;

    up.pid = p->pid;

    safestrcpy(up.name, p->name, sizeof(up.name));
    safestrcpy(up.cwd, p->cwd_str, sizeof(up.cwd));

    if (p->state >= 0 && p->state < NELEM(procstate_names) &&
        procstate_names[p->state])
      safestrcpy(up.state, procstate_names[p->state], sizeof(up.state));
    else
      safestrcpy(up.state, "???", sizeof(up.state));

    if (p->parent != 0) {
      up.ppid = p->parent->pid;
      safestrcpy(up.pname, p->parent->name, sizeof(up.pname));
    } else {
      up.ppid = -1;
      safestrcpy(up.pname, "???", sizeof(up.pname));
    }

    up.size = p->sz;
    up.open_fds = 0;

    for (int fd = 0; fd < NOFILE; fd++) {
      if (p->ofile[fd]) {
        up.open_fds++;
      }
    }

    uproc_list[count++] = up;
  }

  release(&ptable.lock);
  return count;
}

void getcwd(char *cwd) {
  struct proc *curproc = myproc();
  safestrcpy(cwd, curproc->cwd_str, sizeof(curproc->cwd_str));
}

void halt(void) {
  // taken from some where on GitHub
  // this is not working :/
  outw(0xb004, 0x2000);
}

// lab assignment 5
// modified from `wait` above
int wait2(struct ptime *pt) {
  // set all values to -1, for safety checks (if used)
  pt->ctime = pt->retime = pt->rutime = pt->stime = pt->ttime = -1;

  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();

  acquire(&ptable.lock);
  for (;;) {
    // Scan through table looking for exited children.
    havekids = 0;
    for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
      if (p->parent != curproc)
        continue;

      havekids = 1;
      if (p->state == ZOMBIE) {
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;

        // lab assignment 5
        *pt = p->time;

        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if (!havekids || curproc->killed) {
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock); // DOC: wait-sleep
  }
}

void updateProcessTimes(void) {
  struct proc *p;
  acquire(&ptable.lock);

  for (p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
    switch (p->state) {
    case RUNNABLE:
      p->time.retime++;
      break;

    case RUNNING:
      p->time.rutime++;
      break;

    case SLEEPING:
      p->time.stime++;
      break;

    default:
      break;
    }
  }

  release(&ptable.lock);
}
