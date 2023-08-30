#include "fs.h"
#include "stat.h"
#include "user.h"
#include "fcntl.h"

#include "colors.h"

const char types[][10] = {
  "???",
  "dir",
  "file",
  "device",
  "symlink"
};

#define TYPE(type) types[type]

// program flags
int showhidden = 0; // -a

char *fmtname(char *path) {
  static char buf[DIRSIZ + 1];
  char *p;

  // Find first character after last slash.
  for (p = path + strlen(path); p >= path && *p != '/'; p--)
    ;

  p++;

  // Return blank-padded name.
  if (strlen(p) >= DIRSIZ)
    return p;

  memmove(buf, p, strlen(p));
  memset(buf + strlen(p), ' ', DIRSIZ - strlen(p));
  return buf;
}

void ls(char *path, int recurse) {
  char buf[512], *p, target[512];
  int fd;
  struct dirent de;
  struct stat st;

  if (stat(path, S_NOFOLLOW, &st) < 0) {
    bprintf(2, "ls: cannot stat %s\n", path);
    return;
  }

  switch (st.type) {
  case T_FILE:
  case T_DEV:
    bprintf(1, "%s\t %d\t %d\t %s\n", TYPE(st.type), st.ino, st.size, fmtname(path));
    break;

  case T_SYMLINK:
    if (readlink(path, target, sizeof(target)) < 0) {
      bprintf(2, "ls: cannot readlink %s\n", path);
      target[0] = '\0';
    }

    bprintf(1, "%s\t %d\t %d\t %s -> ", TYPE(st.type), st.ino, st.size, colored(fmtname(path), FG_CYAN, COLOR_NONE));
    bprintf(1, "%s\n", colored((target[0] != '\0') ? target : "???", FG_YELLOW, COLOR_NONE));
    break;

  case T_DIR:
    if (!recurse) {
      bprintf(1, "%s\t %d\t %d\t %s\n", TYPE(st.type), st.ino, st.size, colored(fmtname(path), FG_BLUE, COLOR_NONE));
      break;
    }

    if ((fd = opendir(path)) < 0) {
      bprintf(2, "ls: cannot opendir %s\n", path);
      return;
    }

    if (strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf)) {
      bprintf(2, "ls: path too long\n");
      break;
    }

    strcpy(buf, path);
    p = buf + strlen(buf);
    *p++ = '/';

    while (!(readdir(fd, &de) < 0)) {
      if (de.inum == 0)
        continue;

      if (de.name[0] == '.' && !showhidden)
        continue;

      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;

      ls(buf, 0);
    }

    closedir(fd);
    break;

  default:
    setfgcolor(FG_RED);
    bprintf(2, "ls: unkown type(%d) %s\n", st.type, path);
    setfgcolor(COLOR_NONE);
  }
}

int main(int argc, char **argv) {
  int use_cwd = 1;

  // parse the option arguments
  for (int i = 1; i < argc; i++) {
    if (argv[i][0] == '-') {
      char *arg = argv[i] + 1;

      while (*arg != '\0') {
        switch (*arg) {
        case 'a':
          showhidden = 1;
          break;

        default:
          bprintf(2, "Invalid argument: %c in %s\n", *arg, argv[i]);
          exit();
        }

        arg++;
      }
    }
  }

  for (int i = 1; i < argc; i++) {
    if (argv[i][0] != '-') {
      use_cwd = 0;
      ls(argv[i], 1);
    }
  }

  if (use_cwd) {
    ls(".", 1);
  }

  exit();
}
