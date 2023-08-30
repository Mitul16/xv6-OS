// this is an autogenerated file - created by syscall_names.py
#include "syscall.h"

// syscall counts
const int NUM_SYSCALLS = 44;

char *syscall_names[] = {
  [SYS_fork] "fork",
  [SYS_exit] "exit",
  [SYS_wait] "wait",
  [SYS_pipe] "pipe",
  [SYS_read] "read",
  [SYS_kill] "kill",
  [SYS_exec] "exec",
  [SYS_fstat] "fstat",
  [SYS_chdir] "chdir",
  [SYS_dup] "dup",
  [SYS_getpid] "getpid",
  [SYS_sbrk] "sbrk",
  [SYS_sleep] "sleep",
  [SYS_uptime] "uptime",
  [SYS_open] "open",
  [SYS_write] "write",
  [SYS_mknod] "mknod",
  [SYS_unlink] "unlink",
  [SYS_link] "link",
  [SYS_mkdir] "mkdir",
  [SYS_close] "close",
  [SYS_getprocinfo] "getprocinfo",
  [SYS_cmostime] "cmostime",
  [SYS_halt] "halt",
  [SYS_getcwd] "getcwd",
  [SYS_getcpuid] "getcpuid",
  [SYS_pipepoll] "pipepoll",
  [SYS_getppid] "getppid",
  [SYS_is_pipe] "is_pipe",
  [SYS_clrscr] "clrscr",
  [SYS_getcattr] "getcattr",
  [SYS_setcattr] "setcattr",
  [SYS_cflush] "cflush",
  [SYS_getuname] "getuname",
  [SYS_setuname] "setuname",
  [SYS_wait2] "wait2",
  [SYS_stat] "stat",
  [SYS_opendir] "opendir",
  [SYS_readdir] "readdir",
  [SYS_closedir] "closedir",
  [SYS_symlink] "symlink",
  [SYS_readlink] "readlink",
  [SYS_setbgcolor] "setbgcolor",
  [SYS_setfgcolor] "setfgcolor"
};

// syscall function prototypes
extern int sys_fork(void);
extern int sys_exit(void);
extern int sys_wait(void);
extern int sys_pipe(void);
extern int sys_read(void);
extern int sys_kill(void);
extern int sys_exec(void);
extern int sys_fstat(void);
extern int sys_chdir(void);
extern int sys_dup(void);
extern int sys_getpid(void);
extern int sys_sbrk(void);
extern int sys_sleep(void);
extern int sys_uptime(void);
extern int sys_open(void);
extern int sys_write(void);
extern int sys_mknod(void);
extern int sys_unlink(void);
extern int sys_link(void);
extern int sys_mkdir(void);
extern int sys_close(void);
extern int sys_getprocinfo(void);
extern int sys_cmostime(void);
extern int sys_halt(void);
extern int sys_getcwd(void);
extern int sys_getcpuid(void);
extern int sys_pipepoll(void);
extern int sys_getppid(void);
extern int sys_is_pipe(void);
extern int sys_clrscr(void);
extern int sys_getcattr(void);
extern int sys_setcattr(void);
extern int sys_cflush(void);
extern int sys_getuname(void);
extern int sys_setuname(void);
extern int sys_wait2(void);
extern int sys_stat(void);
extern int sys_opendir(void);
extern int sys_readdir(void);
extern int sys_closedir(void);
extern int sys_symlink(void);
extern int sys_readlink(void);
extern int sys_setbgcolor(void);
extern int sys_setfgcolor(void);

// syscalls array
int (*syscalls[])(void) = {
  [SYS_fork] sys_fork,
  [SYS_exit] sys_exit,
  [SYS_wait] sys_wait,
  [SYS_pipe] sys_pipe,
  [SYS_read] sys_read,
  [SYS_kill] sys_kill,
  [SYS_exec] sys_exec,
  [SYS_fstat] sys_fstat,
  [SYS_chdir] sys_chdir,
  [SYS_dup] sys_dup,
  [SYS_getpid] sys_getpid,
  [SYS_sbrk] sys_sbrk,
  [SYS_sleep] sys_sleep,
  [SYS_uptime] sys_uptime,
  [SYS_open] sys_open,
  [SYS_write] sys_write,
  [SYS_mknod] sys_mknod,
  [SYS_unlink] sys_unlink,
  [SYS_link] sys_link,
  [SYS_mkdir] sys_mkdir,
  [SYS_close] sys_close,
  [SYS_getprocinfo] sys_getprocinfo,
  [SYS_cmostime] sys_cmostime,
  [SYS_halt] sys_halt,
  [SYS_getcwd] sys_getcwd,
  [SYS_getcpuid] sys_getcpuid,
  [SYS_pipepoll] sys_pipepoll,
  [SYS_getppid] sys_getppid,
  [SYS_is_pipe] sys_is_pipe,
  [SYS_clrscr] sys_clrscr,
  [SYS_getcattr] sys_getcattr,
  [SYS_setcattr] sys_setcattr,
  [SYS_cflush] sys_cflush,
  [SYS_getuname] sys_getuname,
  [SYS_setuname] sys_setuname,
  [SYS_wait2] sys_wait2,
  [SYS_stat] sys_stat,
  [SYS_opendir] sys_opendir,
  [SYS_readdir] sys_readdir,
  [SYS_closedir] sys_closedir,
  [SYS_symlink] sys_symlink,
  [SYS_readlink] sys_readlink,
  [SYS_setbgcolor] sys_setbgcolor,
  [SYS_setfgcolor] sys_setfgcolor
};

