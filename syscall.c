#include "syscall.h"
#include "defs.h"
#include "memlayout.h"
#include "mmu.h"
#include "param.h"
#include "proc.h"
#include "types.h"
#include "x86.h"

#include "colors.h"

int syscall_debug = 0;

// User code makes a system call with INT T_SYSCALL.
// System call number in %eax.
// Arguments on the stack, from the user call to the C
// library system call function. The saved user %esp points
// to a saved program counter, and then the first argument.

// Fetch the int at addr from the current process.
int fetchint(uint addr, int *ip) {
  struct proc *curproc = myproc();

  if (addr >= curproc->sz || addr + 4 > curproc->sz)
    return -1;

  *ip = *(int *)(addr);
  return 0;
}

// Fetch the nul-terminated string at addr from the current process.
// Doesn't actually copy the string - just sets *pp to point at it.
// Returns length of string, not including nul.
int fetchstr(uint addr, char **pp) {
  char *s, *ep;
  struct proc *curproc = myproc();

  if (addr >= curproc->sz)
    return -1;

  *pp = (char *)addr;
  ep = (char *)curproc->sz;

  for (s = *pp; s < ep; s++) {
    if (*s == 0)
      return s - *pp;
  }

  return -1;
}

// Fetch the nth 32-bit system call argument.
int argint(int n, int *ip) {
  return fetchint((myproc()->tf->esp) + 4 + 4 * n, ip);
}

// Fetch the nth word-sized system call argument as a pointer
// to a block of memory of size bytes.  Check that the pointer
// lies within the process address space.
int argptr(int n, char **pp, int size) {
  int i;
  struct proc *curproc = myproc();

  if (argint(n, &i) < 0)
    return -1;

  if (size < 0 || (uint)i >= curproc->sz || (uint)i + size > curproc->sz)
    return -1;

  *pp = (char *)i;
  return 0;
}

// Fetch the nth word-sized system call argument as a string pointer.
// Check that the pointer is valid and the string is nul-terminated.
// (There is no shared writable memory, so the string can't change
// between this check and being used by the kernel.)
int argstr(int n, char **pp) {
  int addr;

  if (argint(n, &addr) < 0)
    return -1;

  return fetchstr(addr, pp);
}

// defined in `syscall_names.c` via `syscall_names.py`
extern const int NUM_SYSCALLS;
extern int (*syscalls[])(void);
extern char *syscall_names[];

void syscall(void) {
  struct proc *curproc = myproc();
  int num = curproc->tf->eax;

  if (num >= 1 && num <= NUM_SYSCALLS && syscalls[num]) {
    if (syscall_debug) {
      cprintf("%s: %s\n", GREEN("syscall"), colored(syscall_names[num], FG_BLUE, COLOR_NONE));
    }
    curproc->tf->eax = syscalls[num]();
  } else {
    cprintf("%d %s: unknown sys call %d\n", curproc->pid, curproc->name, num);
    curproc->tf->eax = -1;
  }
}
