#ifndef SIGNAL_H
#define SIGNAL_H

#include "proc.h"
#include "types.h"

#define SIGKILL 1
#define SIGSTOP 2

typedef int (*sighandler_t)(void);

void siginit(void);
void signal_handler(struct cpu *);
void sigsend(uint);
void sigret(void);

#endif // SIGNAL_H
