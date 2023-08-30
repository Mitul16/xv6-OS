#ifndef USER_H
#define USER_H

#include "date.h"
#include "proc.h"
#include "stat.h"
#include "fs.h"
#include "types.h"
#include "colors.h"

// system calls
int fork(void);
int exit(void) __attribute__((noreturn));
int wait(void);
int pipe(int *);
int write(int, const void *, int);
int read(int, void *, int);
int close(int);
int kill(int);
int exec(char *, char **);
int open(const char *, int);
int mknod(const char *, short, short);
int unlink(const char *);
int fstat(int fd, struct stat *);
int link(const char *, const char *);
int mkdir(const char *);
int chdir(const char *);
int dup(int);
int getpid(void);
char *sbrk(int);
int sleep(uint);
int uptime(void);

// printf.c
void printf(int, const char *, ...);
void putc(int, const char);

// ulib.c
//int stat(const char *, struct stat *);
char *strcpy(char *, const char *);
void *memmove(void *, const void *, int);
char *strchr(const char *, char c);
char *gets(char *, int max, char);
int strcmp(const char *, const char *);
uint strlen(const char *);
void *memset(void *, int, uint);
void *malloc(uint);
void free(void *);
int atoi(const char *);

// extra
// console.c
void clrscr(void);
void getcattr(int *, int *);
void setcattr(int, int);
void cflush(void);

// lapic.c
void cmostime(struct rtcdate *);

// printf.c
void bprintf(int, const char *, ...);

// pipe.c
int pipepoll(int);

// proc.c
int getprocinfo(void *);
void halt(void);
void getcwd(char *);

// sysfile.c
int is_pipe(int);

// sysproc.c
int getcpuid(void);
int getppid(void);
void setuname(char *);
void getuname(char *);

// ulib.c
char *strncpy(char *, const char *, int);
int strncmp(const char *, const char *, int);
char *strcat(char *, const char *);
char *strncat(char *, const char *, int);
char *itoa(int, char *);
char getc(void);
char *getline(char *, int);
uint random();
void seedrandom(uint);

// lab assignment 5
int wait2(struct ptime *);

// mid-term project
int stat(char *, int, struct stat *);
int opendir(char *);
int readdir(int, struct dirent *);
int closedir(int);
int symlink(char *, char *);
int readlink(char *, char *, uint);

// extra
void setbgcolor(color_t);
void setfgcolor(color_t);

#endif // USER_H
