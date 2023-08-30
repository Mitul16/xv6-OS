# xv6-OS
This is part of my Operating Systems course in the college, where we were introduced to the [xv6](https://pdos.csail.mit.edu/6.828/2019/xv6.html) UNIX-based system to learn about the internal workings of a UNIX-like system.
- How a system boots and handles control to the system kernel
- Inside XV6 Kernel:
  - System Call Interface to interact with the underlying system for memory, storage, process management
  - How are system calls implemented and how to write your own
  - How locks are used to protect shared resources like process tables (`ptable`, see `ptable.lock` in `proc.c`)

- Write various programs for the **XV6** system about:
  - Inter Process Communication (IPC) (see `pctalks*`)
  - File System Usage (see `bigfile`)
  - Signal Handling (hacky implementation for `Ctrl-C`, to send `SIGKILL` signal)
  - Console Interface (ANSI terminal controls, see `man console_codes`)

## Usage
```bash
make qemu-nox

# within the `xv6` system
$ bigfile
.
.
.
$ pctalks3
$ sleep2 100ms
$ wait2
.
.
$ ln -s ls my-ls
$ ln -s not-found symlink2
```
