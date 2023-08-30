// Shell.

#include "date.h"
#include "fcntl.h"
#include "fs.h"
#include "param.h"
#include "proc.h"
#include "stat.h"
#include "user.h"
#include "colors.h"

// Parsed command representation
#define EXEC 1
#define REDIR 2
#define PIPE 3
#define LIST 4
#define BACK 5

#define MAXARGS 10

struct cmd {
  int type;
};

struct execcmd {
  int type;
  char *argv[MAXARGS];
  char *eargv[MAXARGS];
};

struct redircmd {
  int type;
  struct cmd *cmd;
  char *file;
  char *efile;
  int mode;
  int fd;
};

struct pipecmd {
  int type;
  struct cmd *left;
  struct cmd *right;
};

struct listcmd {
  int type;
  struct cmd *left;
  struct cmd *right;
};

struct backcmd {
  int type;
  struct cmd *cmd;
};

int fork1(void); // Fork but panics on failure.
void panic(char *);
struct cmd *parsecmd(char *);

// extra
void builtin_help(const char *), builtin_cd(const char *), builtin_clear(void),
    builtin_ps(void), builtin_pwd(void), builtin_exit(void), builtin_time(void),
    builtin_history(void), builtin_r(void);

const char *shell_builtins[][3] = {
    {"help", "display shell help", (const char *)builtin_help},
    {"cd", "change directory, example: 'cd /path/to/directory'",
     (const char *)builtin_cd},
    {"clear", "clear the screen", (const char *)builtin_clear},
    {"ps", "show process info", (const char *)builtin_ps},
    {"pwd", "print working directory", (const char *)builtin_pwd},
    {"exit", "exit the shell process", (const char *)builtin_exit},
    {"time", "get the current time", (const char *)builtin_time},
    {"history", "get history of most recent commands",
     (const char *)builtin_history},
    {"r", "run the last command", (const char *)builtin_r}};

const int n_builtins = sizeof(shell_builtins) / sizeof(shell_builtins[0]);

// Execute cmd.  Never returns.
void runcmd(struct cmd *cmd) {
  int p[2];
  struct backcmd *bcmd;
  struct execcmd *ecmd;
  struct listcmd *lcmd;
  struct pipecmd *pcmd;
  struct redircmd *rcmd;

  int fd;
  struct stat st;

  if (cmd == 0)
    exit();

  switch (cmd->type) {
  case EXEC:
    ecmd = (struct execcmd *)cmd;

    if (ecmd->argv[0] == 0)
      exit();

    exec(ecmd->argv[0], ecmd->argv);
    setfgcolor(FG_RED);
    bprintf(2, "sh (error): exec %s failed\n", ecmd->argv[0]);
    setfgcolor(COLOR_NONE);
    break;

  case REDIR:
    rcmd = (struct redircmd *)cmd;
    close(rcmd->fd);

    if ((fd = open(rcmd->file, rcmd->mode)) < 0) {
      // `dup2` to use new fd?
      bprintf(2, "sh (error): cannot open %s\n", rcmd->file);
      exit();
    }

    if (fstat(fd, &st) < 0) {
      close(fd);
      bprintf(2, "sh (error): cannot stat %s\n", rcmd->file);
      exit();
    }

    if (st.type != T_FILE) {
      close(fd);
      bprintf(2, "sh (error): %s is not a file\n", rcmd->file);
      exit();
    }

    runcmd(rcmd->cmd);
    break;

  case LIST:
    lcmd = (struct listcmd *)cmd;

    if (fork1() == 0)
      runcmd(lcmd->left);

    wait();
    runcmd(lcmd->right);
    break;

  case PIPE:
    pcmd = (struct pipecmd *)cmd;

    if (pipe(p) < 0)
      panic("pipe");

    if (fork1() == 0) {
      close(1);
      dup(p[1]);
      close(p[0]);
      close(p[1]);
      runcmd(pcmd->left);
    }

    if (fork1() == 0) {
      close(0);
      dup(p[0]);
      close(p[0]);
      close(p[1]);
      runcmd(pcmd->right);
    }

    close(p[0]);
    close(p[1]);
    wait();
    wait();
    break;

  case BACK:
    bcmd = (struct backcmd *)cmd;

    if (fork1() == 0)
      runcmd(bcmd->cmd);

    break;

  default:
    panic("runcmd");
  }

  exit();
}

void showshellprompt(void) {
  static char cwd[128], uname[36];
  getcwd(cwd);
  getuname(uname);

  // observe how `getppid` is called before `getpid`
  // the way stack works - LIFO
  // and compiler saves the values and addresses
  bprintf(
    2,
    "(pid: %d, ppid: %d) %s%s%s:",
    getpid(),
    getppid(),
    colored(uname, FG_GREEN, COLOR_NONE),
    WHITE("@"),
    YELLOW("xv6")
  );

  // to prevent re-use of the first arg - pointers and stack
  bprintf(2, "%s\n$ ", colored(cwd, FG_CYAN, COLOR_NONE));
}

#define C(x) ((x) - '@') // Control-x
#define BACKSPACE 0x100
#define ESC 27
#define TAB 9

// these require `ESC` first
#define CSI 91

// these require `ESC CSI`
#define UP 65
#define DOWN 66
#define RIGHT 67
#define LEFT 68
#define DELETE 51
#define HOME 49
#define END 52

// the above ones were for `tmux` session
// following is from normal terminal session
#define HOME2 72
#define END2 70

#define MAX_HISTORY 16
#define COMMAND_LENGTH 128

#define NULL 0

// to store previously ran commands
int commandHistoryIndex = -1, currentCommandHistoryIndex;
static char commandHistory[MAX_HISTORY][COMMAND_LENGTH];

void appendCommandToHistory(char *command) {
  if (commandHistoryIndex < MAX_HISTORY - 1) {
    commandHistoryIndex++;
  } else {
    for (int i = 0; i < MAX_HISTORY - 1; i++) {
      // memmove(commandHistory, commandHistory+1, (MAX_HISTORY - 1) *
      // COMMAND_LENGTH);
      strcpy(commandHistory[i], commandHistory[i + 1]);
    }
  }

  strcpy(commandHistory[commandHistoryIndex], command);
}

char *getCommandFromHistory(int dir) {
  if ((dir > 0 && currentCommandHistoryIndex > commandHistoryIndex) ||
      (dir < 0 && currentCommandHistoryIndex <= 0))
    return NULL;

  // update current index
  currentCommandHistoryIndex += dir;

  if (currentCommandHistoryIndex > commandHistoryIndex)
    return NULL;

  return commandHistory[currentCommandHistoryIndex];
}

void resetCurrentCommandHistoryIndex() {
  currentCommandHistoryIndex = commandHistoryIndex + 1;
}

void clearCommand(int currentPos, int nread) {
  for (int i = currentPos; i < nread; i++)
    write(1, " ", 1);

  for (int i = currentPos; i < nread; i++)
    write(1, "\b", 1);

  for (int i = 0; i < currentPos; i++)
    write(1, "\b \b", 3);
}

int getEscapeCode(int *org_c) {
  int c = getc();

  if (c == CSI) {
    c = getc();

    switch (c) {
    case UP:
    case DOWN:
    case RIGHT:
    case LEFT:
    case DELETE:
    case HOME:
    case HOME2:
    case END:
    case END2:
      return c;

    default:
      return 1;
    }
  }

  *org_c = c;
  return 0;
}

// 0 - no input, 1 - command, 2 - shell interpretation
// `ctrlL` is moved inside input loop
int getinput(char *buf, int max) {
  int lineCompleted = 0, ctrlL = 0;
  int nread = 0, currentPos = 0;
  int c, code = 0;

  char *historyCommand;
  resetCurrentCommandHistoryIndex();

  // flush any recent input
  cflush();

  // save console attributes
  int echo, buffer;
  getcattr(&echo, &buffer);

  // disable console echoing the input characters
  // and use of input buffer
  setcattr(0, 0);

  while (nread < max && (c = getc())) {
    switch (c) {
    case '\n':
    case '\r':
      lineCompleted = 1;
      break;

    case C('U'):
      clearCommand(currentPos, nread);
      *buf = 0;
      break;

    case C('L'):
      ctrlL = 1;
      break;

    case C('H'):
    case '\x7f':
      // remove backspace and the previous character
      if (currentPos > 0) {
        write(1, "\b \b", 1);
        write(1, buf + currentPos, nread - currentPos);
        write(1, " \b", 2);
        currentPos--;
        nread--;

        memmove(buf + currentPos, buf + currentPos + 1, nread - currentPos);

        for (int i = 0; i < nread - currentPos; i++) {
          putc(1, ESC);
          putc(1, CSI);
          putc(1, LEFT);
        }
      }

      break;

    case ESC:
      code = getEscapeCode(&c);

      // it is a CSI sequence, else print ESC? (or ^[)
      if (code != 0) {
        switch (code) {
        case UP:
          historyCommand = getCommandFromHistory(-1);

          if (historyCommand == NULL)
            break;

          clearCommand(currentPos, nread);
          bprintf(1, historyCommand, COMMAND_LENGTH);

          strcpy(buf, historyCommand);
          nread = currentPos = strlen(historyCommand);
          break;

        case DOWN:
          historyCommand = getCommandFromHistory(+1);

          if (historyCommand == NULL) {
            clearCommand(currentPos, nread);
            nread = currentPos = 0;
          } else {
            clearCommand(currentPos, nread);
            bprintf(1, historyCommand, COMMAND_LENGTH);

            strcpy(buf, historyCommand);
            nread = currentPos = strlen(historyCommand);
          }

          break;

        case RIGHT:
          if (currentPos < nread) {
            putc(1, ESC);
            putc(1, CSI);
            putc(1, RIGHT);
            currentPos++;
          }

          break;

        case LEFT:
          if (currentPos > 0) {
            putc(1, ESC);
            putc(1, CSI);
            putc(1, LEFT);
            currentPos--;
          }

          break;

        case HOME:
        case HOME2:
          while (currentPos > 0) {
            putc(1, ESC);
            putc(1, CSI);
            putc(1, LEFT);
            currentPos--;
          }

          break;

        case END:
        case END2:
          while (currentPos < nread) {
            putc(1, ESC);
            putc(1, CSI);
            putc(1, RIGHT);
            currentPos++;
          }

          break;

        case DELETE:
          if (nread > currentPos) {
            nread--;

            memmove(buf + currentPos, buf + currentPos + 1, nread - currentPos);
            write(1, buf + currentPos, nread - currentPos);
            write(1, " \b", 2);

            for (int i = 0; i < nread - currentPos; i++) {
              putc(1, ESC);
              putc(1, CSI);
              putc(1, LEFT);
            }
          }

          break;

        default:
          break;
          // bprintf(2, "code = %d\n", code);
        }

        break;
      } else {
        // let it pass through to `default` ?
      }

      break;

    // TAB completetion, list files starting with current input buffer
    case TAB:
      break;

    // prevent some keycodes that mess up my work :joy:
    case 126:
      break;

    default:
      putc(1, c);

      memmove(buf + currentPos + 1, buf + currentPos, nread - currentPos);

      buf[currentPos++] = c;
      nread++;

      write(1, buf + currentPos, nread - currentPos);

      for (int i = 0; i < nread - currentPos; i++) {
        putc(1, ESC);
        putc(1, CSI);
        putc(1, LEFT);
      }
    }

    if (ctrlL) {
      ctrlL = 0;
      clrscr();
      showshellprompt();
      write(1, buf, nread);

      for (int i = 0; i < nread - currentPos; i++) {
        putc(1, ESC);
        putc(1, CSI);
        putc(1, LEFT);
      }
    }

    if (lineCompleted) // || ctrlL)
      break;
  }

  *(buf + nread) = '\0';

  // re-enable (if it was enabled) console handling the input as usual
  setcattr(echo, buffer);

  if (nread > 0)
    lineCompleted = 1;

  if (lineCompleted)
    return 1;

  return 0;
}

int getcmd(char *buf, int nbuf) {
  showshellprompt();
  memset(buf, 0, nbuf);
  // gets(buf, nbuf, '\n');
  int result = getinput(buf, nbuf);

  if (result == 2) {
    clrscr();
  }

  return result;
}

int runCommand(char *buf) {
  int result;

  // save the command, what if it didn't ran?
  appendCommandToHistory(buf);

  if ((result = fork1()) == 0)
    runcmd(parsecmd(buf));

  wait();
  return result;
}

int main(void) {
  static char buf[100], *p;
  int fd;

  // Ensure that three file descriptors are open.
  while ((fd = open("console", O_RDWR)) >= 0) {
    if (fd >= 3) {
      close(fd);
      break;
    }
  }

  int builtin;

  // Read and run input commands
  int result;

  while ((result = getcmd(buf, sizeof(buf))) > 0) {
    // it was not a command but some other shell interpretation
    if (result != 1)
      continue;

    putc(1, '\n');
    builtin = 0;

    for (int i = 0; i < n_builtins && !builtin; i++) {
      unsigned int _len = strlen(shell_builtins[i][0]);

      if (strncmp(buf, shell_builtins[i][0], _len) == 0 &&
          (buf[_len] == '\0' || buf[_len] == ' ' || buf[_len] == '\n')) {
        builtin = 1;
        // bprintf(2, "%s: shell built-in command\n", shell_builtins[i][0]);
        ((void (*)(const char *))shell_builtins[i][2])(
            buf + strlen(shell_builtins[i][0]));
      }
    }

    if (!builtin) {
      // run only when the command is not empty
      p = buf;
      while (*p == ' ')
        p++;

      if (strlen(p) > 0 && *p != '\n')
        runCommand(buf);
    }

    putc(1, '\n');
  }

  builtin_exit();
}

void panic(char *s) {
  bprintf(2, "sh (panic): %s\n", s);
  exit();
}

int fork1(void) {
  int pid;

  pid = fork();
  if (pid == -1)
    panic("fork");

  return pid;
}

// Constructors

struct cmd *execcmd(void) {
  struct execcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = EXEC;
  return (struct cmd *)cmd;
}

struct cmd *redircmd(struct cmd *subcmd, char *file, char *efile, int mode,
                     int fd) {
  struct redircmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = REDIR;
  cmd->cmd = subcmd;
  cmd->file = file;
  cmd->efile = efile;
  cmd->mode = mode;
  cmd->fd = fd;
  return (struct cmd *)cmd;
}

struct cmd *pipecmd(struct cmd *left, struct cmd *right) {
  struct pipecmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = PIPE;
  cmd->left = left;
  cmd->right = right;
  return (struct cmd *)cmd;
}

struct cmd *listcmd(struct cmd *left, struct cmd *right) {
  struct listcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = LIST;
  cmd->left = left;
  cmd->right = right;
  return (struct cmd *)cmd;
}

struct cmd *backcmd(struct cmd *subcmd) {
  struct backcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = BACK;
  cmd->cmd = subcmd;
  return (struct cmd *)cmd;
}

// Parsing
char whitespace[] = " \t\r\n\v";
char symbols[] = "<|>&;()";

int gettoken(char **ps, char *es, char **q, char **eq) {
  char *s;
  int ret;

  s = *ps;
  while (s < es && strchr(whitespace, *s))
    s++;

  if (q)
    *q = s;

  ret = *s;
  switch (*s) {
  case 0:
    break;

  case '|':
  case '(':
  case ')':
  case ';':
  case '&':
  case '<':
    s++;
    break;

  case '>':
    s++;
    if (*s == '>') {
      ret = '+';
      s++;
    }
    break;

  default:
    ret = 'a';
    while (s < es && !strchr(whitespace, *s) && !strchr(symbols, *s))
      s++;

    break;
  }

  if (eq)
    *eq = s;

  while (s < es && strchr(whitespace, *s))
    s++;

  *ps = s;
  return ret;
}

int peek(char **ps, char *es, char *toks) {
  char *s;

  s = *ps;
  while (s < es && strchr(whitespace, *s))
    s++;

  *ps = s;
  return *s && strchr(toks, *s);
}

struct cmd *parseline(char **, char *);
struct cmd *parsepipe(char **, char *);
struct cmd *parseexec(char **, char *);
struct cmd *nulterminate(struct cmd *);

struct cmd *parsecmd(char *s) {
  char *es;
  struct cmd *cmd;

  es = s + strlen(s);
  cmd = parseline(&s, es);
  peek(&s, es, "");

  if (s != es) {
    bprintf(2, "leftovers: %s\n", s);
    panic("syntax");
  }

  nulterminate(cmd);
  return cmd;
}

struct cmd *parseline(char **ps, char *es) {
  struct cmd *cmd;

  cmd = parsepipe(ps, es);
  while (peek(ps, es, "&")) {
    gettoken(ps, es, 0, 0);
    cmd = backcmd(cmd);
  }

  if (peek(ps, es, ";")) {
    gettoken(ps, es, 0, 0);
    cmd = listcmd(cmd, parseline(ps, es));
  }

  return cmd;
}

struct cmd *parsepipe(char **ps, char *es) {
  struct cmd *cmd;

  cmd = parseexec(ps, es);
  if (peek(ps, es, "|")) {
    gettoken(ps, es, 0, 0);
    cmd = pipecmd(cmd, parsepipe(ps, es));
  }
  return cmd;
}

struct cmd *parseredirs(struct cmd *cmd, char **ps, char *es) {
  int tok;
  char *q, *eq;

  while (peek(ps, es, "<>")) {
    tok = gettoken(ps, es, 0, 0);

    if (gettoken(ps, es, &q, &eq) != 'a')
      panic("missing file for redirection");

    switch (tok) {
    case '<':
      cmd = redircmd(cmd, q, eq, O_RDONLY, 0);
      break;

    case '>':
      cmd = redircmd(cmd, q, eq, O_WRONLY | O_CREATE, 1);
      break;

    case '+': // >>
      cmd = redircmd(cmd, q, eq, O_WRONLY | O_CREATE, 1);
      break;
    }
  }

  return cmd;
}

struct cmd *parseblock(char **ps, char *es) {
  struct cmd *cmd;

  if (!peek(ps, es, "("))
    panic("parseblock");

  gettoken(ps, es, 0, 0);
  cmd = parseline(ps, es);

  if (!peek(ps, es, ")"))
    panic("syntax - missing )");

  gettoken(ps, es, 0, 0);
  cmd = parseredirs(cmd, ps, es);
  return cmd;
}

struct cmd *parseexec(char **ps, char *es) {
  char *q, *eq;
  int tok, argc;
  struct execcmd *cmd;
  struct cmd *ret;

  if (peek(ps, es, "("))
    return parseblock(ps, es);

  ret = execcmd();
  cmd = (struct execcmd *)ret;

  argc = 0;
  ret = parseredirs(ret, ps, es);

  while (!peek(ps, es, "|)&;")) {
    if ((tok = gettoken(ps, es, &q, &eq)) == 0)
      break;

    if (tok != 'a')
      panic("syntax");

    cmd->argv[argc] = q;
    cmd->eargv[argc] = eq;
    argc++;

    if (argc >= MAXARGS)
      panic("too many args");

    ret = parseredirs(ret, ps, es);
  }

  cmd->argv[argc] = 0;
  cmd->eargv[argc] = 0;
  return ret;
}

// NULL-terminate all the counted strings.
struct cmd *nulterminate(struct cmd *cmd) {
  int i;
  struct backcmd *bcmd;
  struct execcmd *ecmd;
  struct listcmd *lcmd;
  struct pipecmd *pcmd;
  struct redircmd *rcmd;

  if (cmd == 0)
    return 0;

  switch (cmd->type) {
  case EXEC:
    ecmd = (struct execcmd *)cmd;
    for (i = 0; ecmd->argv[i]; i++)
      *ecmd->eargv[i] = 0;

    break;

  case REDIR:
    rcmd = (struct redircmd *)cmd;
    nulterminate(rcmd->cmd);
    *rcmd->efile = 0;
    break;

  case PIPE:
    pcmd = (struct pipecmd *)cmd;
    nulterminate(pcmd->left);
    nulterminate(pcmd->right);
    break;

  case LIST:
    lcmd = (struct listcmd *)cmd;
    nulterminate(lcmd->left);
    nulterminate(lcmd->right);
    break;

  case BACK:
    bcmd = (struct backcmd *)cmd;
    nulterminate(bcmd->cmd);
    break;
  }

  return cmd;
}

// extra
char *ljust(char *dest, const char *src, const char fillchar, int len) {
  char *d = dest;

  while (len > 0 && *src) {
    *dest++ = *src++;
    len--;
  }

  while (len > 0) {
    *dest++ = fillchar;
    len--;
  }

  *dest = 0;
  return d;
}

void builtin_help(const char *buf) {
  // `arg` is restricted to `a-z` only
  int argc = 0;
  char arg[10], temp[10];

  // don't get it, why do I need to nullify the memory
  // in normal programs, this is not required
  memset(arg, 0, sizeof(arg));

  while (*buf && argc < 2) {
    while (*buf && (*buf < 'a' || *buf > 'z'))
      buf++;

    if (argc < 1) {
      for (int i = 0; *buf && i < sizeof(arg); i++, buf++) {
        if (*buf >= 'a' && *buf <= 'z') {
          arg[i] = *buf;
        } else
          break;
      }
    }

    while (*buf == ' ')
      buf++;

    argc++;
  }

  // ensure the `arg` string is NULL-terminated
  // arg[sizeof(arg) - 1] = 0;

  if (argc == 0) {
    for (int i = 0; i < n_builtins; i++) {
      bprintf(1, "\t%s: %s\n", ljust(temp, shell_builtins[i][0], ' ', 10),
              shell_builtins[i][1]);
    }

    return;
  }

  if (argc == 1) {
    for (int i = 0; i < n_builtins; i++) {
      if (strcmp(arg, shell_builtins[i][0]) == 0) {
        bprintf(1, "help %s: %s\n", shell_builtins[i][0], shell_builtins[i][1]);
        return;
      }
    }
  }

  bprintf(2, "usage:\n\thelp\n\thelp <shell built-in command>\n");
}

void builtin_cd(const char *buf) {
  while (*buf == ' ')
    buf++;

  int len = strlen(buf);
  char dir[64];
  strncpy(dir, buf, sizeof(dir));

  while (dir[len - 1] == '\n')
    dir[--len] = 0;

  if (dir[0] == 0)
    strncpy(dir, "/", 2);

  if (chdir(dir) < 0)
    bprintf(2, "cannot cd %s\n", dir);
}

void builtin_clear(void) { clrscr(); }

void builtin_ps(void) {
  // there is some issue if we run this without fork
  // possibly because current process state is `sleep`
  // or we couldn't allocate memory
  // we get some trap error 14, page fault if run without `fork`

  // using `static struct uproc ...` resolved this issue, it was perhaps caused
  // due to memory allocations

  static struct uproc uproc_list[NPROC], *up;
  int proccount = getprocinfo((void *)uproc_list), len;
  char temp[44];

  bprintf(1,
          "PID\tNAME\t\tCWD\t\t\t\t\tSTATE\tPPID\tPNAME\t\tSIZE\tOPEN fd(s)\n");

  if (proccount > 0) {
    for (up = uproc_list; up < &uproc_list[proccount]; up++) {
      strcpy(up->name, ljust(temp, up->name, ' ', 16));
      strcpy(up->state, ljust(temp, up->state, ' ', 8));
      strcpy(up->pname, ljust(temp, up->pname, ' ', 16));

      if ((len = strlen(up->cwd)) >= 40) {
        strncpy(up->cwd, "...", 3);

        for (int i = 3; i < 40; i++)
          up->cwd[i] = up->cwd[len - 39 + i];
      } else {
        strcpy(up->cwd, ljust(temp, up->cwd, ' ', 40));
      }

      up->cwd[39] = 0;

      bprintf(1, "%s\t%s%s %s%s\t%s%d\t%d\n", itoa(up->pid, temp), up->name,
              up->cwd, up->state, up->ppid > 0 ? itoa(up->ppid, temp) : "???",
              up->pname, up->size, up->open_fds);
    }
  } else {
    bprintf(2, "ps: no info received from 'getprocinfo'\n");
  }
}

void builtin_pwd(void) {
  static char cwd[128];
  getcwd(cwd);
  bprintf(1, "%s\n", cwd);
}

void builtin_exit(void) {
  bprintf(2, "exit\n");
  exit();
}

void builtin_time(void) {
  struct rtcdate current;
  cmostime(&current);

  // convert to UTC +5:30 (just a quick fix)
  int dminute = 30, dhour = 5;

  if (current.minute <= 60 - dminute) {
    current.minute += dminute;
  } else {
    current.minute -= 60 - dminute;
    dhour++;
  }

  if (current.hour <= 24 - dhour) {
    current.hour += dhour;
  } else {
    current.hour -= 24 - dhour;
    int maxdays = 0;

    switch (current.month) {
    case 1:
    case 3:
    case 5:
    case 7:
    case 8:
    case 10:
    case 12:
      maxdays = 31;
      break;

    case 2:
      maxdays = 28 + ((current.year % 4 == 0 && current.year % 100 != 0) ||
                      current.year % 400 == 0); // check for leap year
      break;

    default:
      maxdays = 30;
    }

    if (current.day < maxdays) {
      current.day += 1;
    } else {
      if (current.month < 12) {
        current.month += 1;
      } else {
        current.month -= 12 - 1;
        current.year++;
      }
    }
  }

  // bprintf(1, "%d second, %d minute, %d hour - %d/%d/%d\n", current);
  bprintf(1, "%d:%d:%d - %d/%d/%d\n", current.hour, current.minute,
          current.second, current.day, current.month, current.year);
}

void builtin_history(void) {
  if (commandHistoryIndex < 0) {
    bprintf(1, "(no history!)\n");
  } else {
    for (int i = 0; i <= commandHistoryIndex; i++) {
      bprintf(1, "%d: %s\n", i + 1, commandHistory[i]);
    }
  }
}

void builtin_r(void) {
  if (commandHistoryIndex < 0) {
    bprintf(1, "(no history!)\n");
  } else {
    char *command = commandHistory[commandHistoryIndex];
    bprintf(1, "running: %s\n", command);
    runCommand(command);
  }
}
