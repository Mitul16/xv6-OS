#ifndef DEFS_H
#define DEFS_H

#include "types.h"
#include "colors.h"
#include "buf.h"
#include "date.h"
#include "file.h"
#include "fs.h"
#include "sleeplock.h"
#include "spinlock.h"
#include "stat.h"
#include "proc.h"

// number of elements in fixed-size array
#define NELEM(x) (sizeof(x) / sizeof((x)[0]))

//struct buf;
//struct context;
//struct file;
//struct inode;
//struct pipe;
//struct proc;
//struct spinlock;
//struct sleeplock;
//struct stat;
//struct superblock;
//struct rtcdate;

extern int uart;

// lab assignment 5
struct ptime;


// bio.c
void binit(void);
struct buf *bread(uint, uint);
void brelse(struct buf *);
void bwrite(struct buf *);

// console.c
void consoleinit(void);
void cprintf(char *, ...);
void consoleintr(int (*)(void));
void panic(char *, char *) __attribute__((noreturn));

// exec.c
int exec(char *, char **);

// file.c
struct file *filealloc(void);
void fileclose(struct file *);
struct file *filedup(struct file *);
void fileinit(void);
int fileread(struct file *, char *, int n);
int filestat(struct file *, struct stat *);
int filewrite(struct file *, char *, int n);

// fs.c
void readsb(int dev, struct superblock *sb);
int dirlink(struct inode *, char *, uint);
struct inode *dirlookup(struct inode *, char *, uint *);
struct inode *ialloc(uint, short);
struct inode *idup(struct inode *);
void iinit(int dev);
void ilock(struct inode *);
void iput(struct inode *);
void iunlock(struct inode *);
void iunlockput(struct inode *);
void iupdate(struct inode *);
int namecmp(const char *, const char *);
struct inode *namei(char *, char);
struct inode *nameiparent(char *, char *);
int readi(struct inode *, char *, uint, uint);
void stati(struct inode *, struct stat *);
int writei(struct inode *, char *, uint, uint);
char *skipelem(char *, char *);

// ide.c
void ideinit(void);
void ideintr(void);
void iderw(struct buf *);

// ioapic.c
void ioapicenable(int irq, int cpu);
extern uchar ioapicid;
void ioapicinit(void);

// kalloc.c
char *kalloc(void);
void kfree(char *);
void kinit1(void *, void *);
void kinit2(void *, void *);

// kbd.c
void kbdintr(void);

// lapic.c
void cmostime(struct rtcdate *r);
int lapicid(void);
extern volatile uint *lapic;
void lapiceoi(void);
void lapicinit(void);
void lapicstartap(uchar, uint);
void microdelay(int);

// log.c
void initlog(int dev);
void log_write(struct buf *);
void begin_op();
void end_op();

// mp.c
extern int ismp;
void mpinit(void);

// picirq.c
void picenable(int);
void picinit(void);

// pipe.c
int pipealloc(struct file **, struct file **);
void pipeclose(struct pipe *, int);
int piperead(struct pipe *, char *, int);
int pipewrite(struct pipe *, char *, int);

// proc.c
int cpuid(void);
void exit(void);
int fork(void);
int growproc(int);
int kill(int);
struct cpu *mycpu(void);
struct proc *myproc();
void pinit(void);
void procdump(void);
void scheduler(void) __attribute__((noreturn));
void sched(void);
void setproc(struct proc *);
void sleep(void *, struct spinlock *);
void userinit(void);
int wait(void);
void wakeup(void *);
void yield(void);

// swtch.S
void swtch(struct context **, struct context *);

// spinlock.c
void acquire(struct spinlock *);
void getcallerpcs(void *, uint *);
int holding(struct spinlock *);
void initlock(struct spinlock *, char *);
void release(struct spinlock *);
void pushcli(void);
void popcli(void);

// sleeplock.c
void acquiresleep(struct sleeplock *);
void releasesleep(struct sleeplock *);
int holdingsleep(struct sleeplock *);
void initsleeplock(struct sleeplock *, char *);

// string.c
int memcmp(const void *, const void *, uint);
void *memmove(void *, const void *, uint);
void *memset(void *, int, uint);
char *safestrcpy(char *, const char *, int);
int strlen(const char *);
int strcmp(const char *, const char *);
int strncmp(const char *, const char *, uint);
char *strncpy(char *, const char *, int);
void *memcpy(void *, const void *, uint);

// syscall.c
int argint(int, int *);
int argptr(int, char **, int);
int argstr(int, char **);
int fetchint(uint, int *);
int fetchstr(uint, char **);
void syscall(void);

// where is this file?
// timer.c
void timerinit(void);

// trap.c
void idtinit(void);
extern uint ticks;
void tvinit(void);
extern struct spinlock tickslock;

// uart.c
void uartinit(void);
void uartintr(void);
void uartputc(int);

// vm.c
void seginit(void);
void kvmalloc(void);
pde_t *setupkvm(void);
char *uva2ka(pde_t *, char *);
int allocuvm(pde_t *, uint, uint);
int deallocuvm(pde_t *, uint, uint);
void freevm(pde_t *);
void inituvm(pde_t *, char *, uint);
int loaduvm(pde_t *, char *, struct inode *, uint, uint);
pde_t *copyuvm(pde_t *, uint);
void switchuvm(struct proc *);
void switchkvm(void);
int copyout(pde_t *, uint, void *, uint);
void clearpteu(pde_t *pgdir, char *uva);

// extra
// console.c
void clrscr(void);
void cgaclear(void);
void uartclear(void);
void cgamovecursor(int);
void uartmovecursor(int);
void getcattr(int *, int *);
void setcattr(int, int);
void flushstdin(void);
void setbgcolor(color_t);
void setfgcolor(color_t);

// fs.c
char *joinpath(char *, char *, char *);

// pipe.c
int pipepoll(struct pipe *);

// proc.c
int getprocinfo(void *);
void getcwd(char *);
void halt(void);

// signal.c
extern void sigsend(uint);
extern void signal_handler(struct cpu *);

// string.c
char *strcat(char *, const char *);
char *strncat(char *, const char *, int);

// sysproc.c
int sys_uptime(void);

// lab assignment 5
int wait2(struct ptime *);
void updateProcessTimes(void);

#endif // DEFS_H
