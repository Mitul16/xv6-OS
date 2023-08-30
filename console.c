// Console input and output.
// Input is from the keyboard or serial port.
// Output is written to the screen and serial port.

#include "defs.h"
#include "file.h"
#include "fs.h"
#include "memlayout.h"
#include "proc.h"
#include "signal.h"
#include "spinlock.h"
#include "traps.h"
#include "types.h"
#include "x86.h"

#include "colors.h"

// modify `gets` and `getline` to handle arrow keys

// extra
extern int uart, syscall_debug;

// echo input characters and use an input buffer
static int __console_echo = 1, __console_buffer = 1;

void clrscr(void), cgaclear(void), uartclear(void), cgamovecursor(int),
    uartmovecursor(int), wakeupconsoleinput(char), flushstdin(void),
    getcattr(int *, int *), setcattr(int, int);

static void consputc(int);
static int panicked = 0;

static struct {
  struct spinlock lock;
  int locking;
} cons;

static void printint(int xx, int base, int sign) {
  static char charset[] = "0123456789abcdef";
  char buf[16];
  int i;
  uint x;

  if (sign && (sign = xx < 0))
    x = -xx;
  else
    x = xx;

  i = 0;
  do {
    buf[i++] = charset[x % base];
  } while ((x /= base) != 0);

  if (sign)
    buf[i++] = '-';

  while (--i >= 0)
    consputc(buf[i]);
}

// Print to the console. only understands %d, %x, %p, %s.
void cprintf(char *fmt, ...) {
  int i, c, locking;
  uint *argp;
  char *s;

  locking = cons.locking;
  if (locking)
    acquire(&cons.lock);

  if (fmt == 0)
    panic("cprintf", "null fmt");

  argp = (uint *)(&fmt + 1);
  for (i = 0; (c = fmt[i] & 0xff) != 0; i++) {
    if (c != '%') {
      consputc(c);
      continue;
    }

    c = fmt[++i] & 0xff;
    if (c == 0)
      break;

    switch (c) {
    case 'd':
      printint(*argp++, 10, 1);
      break;

    case 'x':
    case 'p':
      printint(*argp++, 16, 0);
      break;

    case 's':
      if ((s = (char *)*argp++) == 0)
        s = "(null)";

      for (; *s; s++)
        consputc(*s);

      break;

    case '%':
      consputc('%');
      break;

    default:
      // Print unknown % sequence to draw attention.
      consputc('%');
      consputc(c);
    }
  }

  if (locking)
    release(&cons.lock);
}

void panic(char *title, char *desc) {
  int i;
  uint pcs[10];

  cli();
  cons.locking = 0;

  // use lapiccpunum so that we can call panic from mycpu()
  cprintf("lapicid %d: panic: ", lapicid());

  if (desc != 0)
    cprintf("%s: %s\n", title, desc);
  else
    cprintf("%s\n", title);

  getcallerpcs(&title, pcs);

  for (i = 0; i < 10; i++)
    cprintf(" %p", pcs[i]);

  panicked = 1; // freeze other CPU
  for (;;)
    ;
}

#define BACKSPACE 0x100
#define CRTPORT 0x3d4
static ushort *crt = (ushort *)P2V(0xb8000); // CGA memory

// this is to suppress compiler warning
// I am not using xterm for CGA output
static void cgaputc(int) __attribute__((unused));

static void cgaputc(int c) {
  int pos;

  // Cursor position: col + 80*row.
  outb(CRTPORT, 14);
  pos = inb(CRTPORT + 1) << 8;
  outb(CRTPORT, 15);
  pos |= inb(CRTPORT + 1);

  if (c == '\n')
    pos += 80 - pos % 80;
  else if (c == BACKSPACE) {
    if (pos > 0)
      --pos;
  } else
    crt[pos++] = (c & 0xff) | 0x0700; // black on white

  if (pos < 0 || pos > 25 * 80)
    panic("cgaputc", "pos under/overflow");

  if ((pos / 80) >= 24) { // Scroll up.
    memmove(crt, crt + 80, sizeof(crt[0]) * 23 * 80);
    pos -= 80;
    memset(crt + pos, 0, sizeof(crt[0]) * (24 * 80 - pos));
  }

  outb(CRTPORT, 14);
  outb(CRTPORT + 1, pos >> 8);
  outb(CRTPORT, 15);
  outb(CRTPORT + 1, pos);
  crt[pos] = ' ' | 0x0700;
}

void consputc(int c) {
  if (panicked) {
    cli();
    for (;;)
      ;
  }

  if (c == BACKSPACE) {
    uartputc('\b');
    uartputc(' ');
    uartputc('\b');
  } else
    uartputc(c);

  // this just adds some overhead, I use `qemu-nox` (without the CGA output)
  // cgaputc(c);
}

#define INPUT_BUF 128
struct {
  char buf[INPUT_BUF];
  uint r; // Read index
  uint w; // Write index
  uint e; // Edit index
} input;

#define C(x) ((x) - '@') // Control-x

// there should be some way to identify the listener
// which is waiting for the console input
// the code below is modified to echo back characters as soon as they arrive

void consoleintr(int (*getc)(void)) {
  int c, do_procdump = 0;

  // extra
  int do_ctrlC = 0, do_ctrlZ = 0;
  int toggle_syscall_debug = 0;

  acquire(&cons.lock);

  // this can be done in a better way
  // but it works for now

  if (__console_buffer) {
    while ((c = getc()) >= 0) {
      switch (c) {
      case C('P'): // Process listing.
        // procdump() locks cons.lock indirectly; invoke later
        do_procdump = 1;
        break;

      case C('U'): // Kill line.
        while (input.e != input.w &&
               input.buf[(input.e - 1) % INPUT_BUF] != '\n') {
          input.e--;

          if (__console_echo)
            consputc(BACKSPACE);
        }
        break;

      case C('H'):
      case '\x7f': // Backspace
        if (input.e != input.w) {
          input.e--;

          if (__console_echo)
            consputc(BACKSPACE);
        }
        break;

      case C('C'):
        do_ctrlC = 1;
        break;

      case C('Z'):
        do_ctrlZ = 1;
        break;

      case C('S'):
        toggle_syscall_debug = 1;
        break;

      default:
        // is this all correct?
        if (c != 0 && input.e - input.r < INPUT_BUF) {
          c = (c == '\r') ? '\n' : c;
          input.buf[input.e++ % INPUT_BUF] = c;

          // restrict to printable characters here?
          // or conver them to `^[` (similar)
          // create an array `printable` and use `0/1` for each character
          if (__console_echo)
            consputc(c);

          if (c == '\n' || c == C('D') || input.e == input.r + INPUT_BUF) {
            input.w = input.e;
            wakeup(&input.r);
          }
        }
      }

      // send NULL to close the input
      if (do_ctrlC)
        c = 0;
    }
  } else {
    c = getc();

    switch (c) {
    case C('P'): // Process listing.
      // procdump() locks cons.lock indirectly; invoke later
      do_procdump = 1;
      break;

    case C('C'):
      do_ctrlC = 1;
      break;

    case C('Z'):
      do_ctrlZ = 1;
      break;

    case C('S'):
      toggle_syscall_debug = 1;
      break;

    case C('H'):
    case '\x7f':
      break;

    default:
      c = (c == '\r') ? '\n' : c;
    }

    // send NULL to close the input
    if (do_ctrlC)
      c = 0;

    if (!(do_ctrlZ || do_procdump || toggle_syscall_debug))
      wakeupconsoleinput(c);
  }

  release(&cons.lock);

  // now `cons.lock` is not held, we can use `cprintf`
  if (do_procdump) {
    procdump();
  }

  // extra
  if (do_ctrlC)
    sigsend(SIGKILL);

  if (do_ctrlZ)
    sigsend(SIGSTOP);

  if (toggle_syscall_debug) {
    syscall_debug ^= 1;
    setfgcolor(FG_YELLOW);
    cprintf("\nconsole: syscall debug %sabled\n", syscall_debug ? "en" : "dis");
    setfgcolor(COLOR_NONE);
  }
}

int consoleread(struct inode *ip, char *dst, int n) {
  uint target;
  int c;

  iunlock(ip);
  target = n;
  acquire(&cons.lock);

  while (n > 0) {
    while (input.r == input.w) {
      if (myproc()->killed) {
        release(&cons.lock);
        ilock(ip);
        return -1;
      }

      sleep(&input.r, &cons.lock);
    }

    c = input.buf[input.r++ % INPUT_BUF];

    if (c == C('D')) { // EOF
      if (n < target) {
        // Save ^D for next time, to make sure
        // caller gets a 0-byte result.
        input.r--;
      }
      break;
    }

    *dst++ = c;
    --n;

    if (c == '\n')
      break;
  }

  release(&cons.lock);
  ilock(ip);

  return target - n;
}

int consolewrite(struct inode *ip, char *buf, int n) {
  int i;

  iunlock(ip);
  acquire(&cons.lock);
  for (i = 0; i < n; i++)
    consputc(buf[i] & 0xff);

  release(&cons.lock);
  ilock(ip);

  return n;
}

void consoleinit(void) {
  initlock(&cons.lock, "console");

  devsw[CONSOLE].write = consolewrite;
  devsw[CONSOLE].read = consoleread;
  cons.locking = 1;

  ioapicenable(IRQ_KBD, 0);
}

// extra
#define UP 65
#define DOWN 66
#define RIGHT 67
#define LEFT 68

// how to find the terminal size?
// count how many '/n' have been printed till now and then print that many lines
// and reset to 0
#define UART_MAXLINES 80

void getcattr(int *echo, int *buffer) {
  *echo = __console_echo;
  *buffer = __console_buffer;
}

void setcattr(int echo, int buffer) {
  __console_echo = echo;
  __console_buffer = buffer;
}

void flushstdin(void) {
  // clears the buffer and resets the index counters (r, e, w)
  memset(&input, 0, sizeof(input));
}

// this might not be safe or some checks can be done like locks
// can be used to solve the multiple console input listeners issue with multiple
// `sh` processes
void wakeupconsoleinput(char c) {
  input.buf[input.e++ % INPUT_BUF] = c;
  input.w = input.e;
  wakeup(&input.r);
}

// check for key press and releas, to only perform an action on specific input
void clrscr(void) {
  cgaclear();
  uartclear();
}

int isValidDirection(int dir) {
  switch (dir) {
  case UP:
  case DOWN:
  case RIGHT:
  case LEFT:
    break;

  default:
    return 0;
  }

  return 1;
}

// for text editor, save the neigbhouring characters (check for bounds)
// and update them after moving the cursor
void cgamovecursor(int dir) {
  // cga stuff here
}

void uartmovecursor(int dir) {
  if (uart && isValidDirection(dir)) {
    uartputc(27);
    uartputc(91);
    uartputc(dir);
  }
}

void cgaclear(void) {
  // check if cga is available

  memset(crt, 0, sizeof(crt[0]) * 25 * 80);
  outb(CRTPORT, 14);
  outb(CRTPORT + 1, 0);
  outb(CRTPORT, 15);
  outb(CRTPORT + 1, 0);
}

void uartclear(void) {
  /*
  for (int i = 0; i < UART_MAXLINES; i++)
    uartputc('\n');

  for (int i = 0; i < UART_MAXLINES; i++)
    uartmovecursor(UP);
  */

  cprintf("\33[H\33[2J");
}

// colors
int sys_setbgcolor(void) {
  color_t bg_color;
  if (argint(0, (int *) &bg_color) < 0)
    return -1;

  setbgcolor(bg_color);
  return 0;
}

int sys_setfgcolor(void) {
  color_t fg_color;
  if (argint(0, (int *) &fg_color) < 0)
    return -1;

  setfgcolor(fg_color);
  return 0;
}

void setbgcolor(color_t bg_color) {
  switch (bg_color) {
  case BG_RED:
    cprintf(_BG_RED);
    break;

  case BG_GREEN:
    cprintf(_BG_GREEN);
    break;

  case BG_YELLOW:
    cprintf(_BG_YELLOW);
    break;

  case BG_BLUE:
    cprintf(_BG_BLUE);
    break;

  case BG_CYAN:
    cprintf(_BG_CYAN);
    break;

  case BG_WHITE:
    cprintf(_BG_WHITE);
    break;

  default:
    cprintf(_BG_RESET);
  }
}

void setfgcolor(color_t fg_color) {
  switch (fg_color) {
  case FG_RED:
    cprintf(_FG_RED);
    break;

  case FG_GREEN:
    cprintf(_FG_GREEN);
    break;

  case FG_YELLOW:
    cprintf(_FG_YELLOW);
    break;

  case FG_BLUE:
    cprintf(_FG_BLUE);
    break;

  case FG_CYAN:
    cprintf(_FG_CYAN);
    break;

  case FG_WHITE:
    cprintf(_FG_WHITE);
    break;

  default:
    cprintf(_FG_RESET);
  }
}
