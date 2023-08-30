// init: The initial user-level program

#include "fcntl.h"
#include "stat.h"
#include "time.h"
#include "user.h"

char *argv[] = {"sh", 0};
char name[32];

void enterprompt(void) {
  bprintf(1, "\npress enter to continue!\n");
  cflush();
  setcattr(0, 0);
  while (getc() != '\n')
    ;
  setcattr(1, 1);
}

int readname(void) {
  int fd;
  struct stat st;

  bprintf(1, "checking saved username...\n");

  if ((fd = open(".username", O_RDONLY)) > 0) {
    // make sure it is a file
    if (fstat(fd, &st) < 0) {
      bprintf(2, "Failed to stat `.username`\n");
      return -2;
    }

    if (st.type != T_FILE) {
      bprintf(2, "failed to read from `.username`, not a file\n");
      bprintf(1, "please remove `.username` to allow saving the username\n");
      bprintf(2, "I will take serious actions in my next kernel release :p\n");
      return -2;
    }

    if (read(fd, name, sizeof(name)) > 0) {
      clrscr();
      bprintf(1, "Welcome %s\n", name);
      pause2(1000);
      cflush();
    } else {
      // couldn't read any data, possibly a blank file
      return -1;
    }

    close(fd);
    return 0;
  }

  return -1;
}

void savename(void) {
  int fd;

  bprintf(1, "saving username...\n");

  if ((fd = open(".username", O_CREATE | O_WRONLY)) > 0) {
    write(fd, name, sizeof(name));
    close(fd);

    bprintf(1, "username saved!\n");
    pause2(1000);
  } else {
    // couldn't create the file
    bprintf(2, "failed to create `.username` file\n");
    bprintf(1, "username is not saved!\n");
    enterprompt();
  }
}

int main(void) {
  int pid, wpid;

  // for username
  int result, len;
  char c;

  // use time value to seed the PRNG
  seedrandom(uptime());

  struct rtcdate now;
  cmostime(&now);
  seedrandom(rtcToseconds(now));

  // for console I/O
  if (open("console", O_RDWR) < 0) {
    mknod("console", 1, 1);
    open("console", O_RDWR);
  }

  dup(0); // stdout
  dup(0); // stderr

  enterprompt();
  clrscr();

  setcattr(0, 0);

  // read the saved username
  if ((result = readname()) == 0)
    goto alreadysaved;
  // couldn't read the saved `.username`
  else if (result == -1) {
    bprintf(1, "no saved username found!\n");
    pause2(1000);

  ask:
    clrscr();
    bprintf(1, "Please enter your username: ");

    cflush();
    setcattr(1, 1);
    getline(name, sizeof(name) - 1);

    while (*name == ' ')
      memmove(name, name + 1, sizeof(name));

    // restrict to alphanums only?
    // will have to check the data from the file as well
    // in case the user updated it
    // or just protect it from modifications
    // and better hide it

    len = strlen(name);
    while (name[len - 1] == ' ')
      name[--len] = '\0';

    if (len > 0) {
      cflush();
      bprintf(1, "%s, is it okay? [y/N] ", name);

      cflush();
      setcattr(0, 0);
      c = getc();
      setcattr(1, 1);

      if (c != 'y' && c != 'Y')
        goto ask;

      goto save;
    }

    bprintf(1, "Received empty input!\n\n");
  }
  // failed to read the username, some issue with the file
  else if (result == -2) {
    enterprompt();
    clrscr();
  }

  strcpy(name, "user");
  bprintf(1, "Selecting default username: user\n");
  bprintf(1, "You can reset this by removing `.username` file\n");

  enterprompt();

save:
  clrscr();
  savename();

alreadysaved:
  setuname(name);
  setcattr(1, 1);

  for (;;) {
    clrscr();
    bprintf(1, "init: starting sh\n");

    if ((pid = fork()) < 0) {
      bprintf(1, "init: fork failed\n");
      exit();
    }

    if (pid == 0) {
      exec("sh", argv);
      bprintf(1, "init: exec sh failed\n");
      exit();
    }

    // for my child and orphans
    while ((wpid = wait()) >= 0 && wpid != pid)
      bprintf(1, "zombie!\n");
  }
}
