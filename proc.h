// Per-CPU state
#ifndef PROC_H
#define PROC_H

#include "mmu.h"
#include "param.h"
#include "types.h"

struct cpu {
  uchar apicid;              // Local APIC ID
  struct context *scheduler; // swtch() here to enter scheduler
  struct taskstate ts;       // Used by x86 to find stack for interrupt
  struct segdesc gdt[NSEGS]; // x86 global descriptor table
  volatile uint started;     // Has the CPU started?
  int ncli;                  // Depth of pushcli nesting.
  int intena;                // Were interrupts enabled before pushcli?
  struct proc *proc;         // The process running on this cpu or null

  // extra
  uint signum;
  struct proc *lastproc;
};

extern struct cpu cpus[NCPU];
extern int ncpu;

// Saved registers for kernel context switches.
// Don't need to save all the segment registers (%cs, etc),
// because they are constant across kernel contexts.
// Don't need to save %eax, %ecx, %edx, because the
// x86 convention is that the caller has saved them.
// Contexts are stored at the bottom of the stack they
// describe; the stack pointer is the address of the context.
// The layout of the context matches the layout of the stack in swtch.S
// at the "Switch stacks" comment. Switch doesn't save eip explicitly,
// but it is on the stack and allocproc() manipulates it.
struct context {
  uint edi;
  uint esi;
  uint ebx;
  uint ebp;
  uint eip;
};

enum procstate { UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

// Process time info
struct ptime {
  // lab assignment 5
  uint ctime, // creation time
      stime,  // sleeping time
      retime, // runnable time
      rutime, // running time
      ttime;  // termination time
};

// Per-process state
struct proc {
  uint sz;                    // Size of process memory (bytes)
  pde_t *pgdir;               // Page table
  char *kstack;               // Bottom of kernel stack for this process
  enum procstate state;       // Process state
  int pid;                    // Process ID
  struct proc *parent;        // Parent process
  struct trapframe *tf;       // Trap frame for current syscall
  struct context *context;    // swtch() here to run process
  void *chan;                 // If non-zero, sleeping on chan
  int killed;                 // If non-zero, have been killed
  struct file *ofile[NOFILE]; // Open files
  struct inode *cwd;          // Current directory
  char name[16];              // Process name (debugging)

  // extra
  char cwd_str[128];
  int cpuid;

  // lab assignment 5
  struct ptime time;
};

// Process info for user space
struct uproc {
  int pid, ppid, open_fds;
  char name[18], pname[18], cwd[128], state[10];
  uint size;
  struct ptime time;

  // why does using cwd[size] for some sizes cause `trap error`?
  // - possibly because we are allocating that memory in `sh.c`
  //   as `struct uproc uproc_list[NPROC];`
  //   and we couldn't allocate the memory and accessed a null pointer
  //   causing the page fault for the same
  //
  // - using a smaller size than `NPROC` didn't cause the same `trap error`s
  //
  // - declaring it as `static` worked
};

// Process memory is laid out contiguously, low addresses first:
//   text
//   original data and bss
//   fixed-size stack
//   expandable heap

#endif // PROC_H
