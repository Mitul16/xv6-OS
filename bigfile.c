#include "fcntl.h"
#include "fs.h"
#include "types.h"
#include "user.h"

#include "colors.h"

#define BIGFILENAME "bigfile.bin"

int main(int argc, char *argv[]) {
  int fd;

  bprintf(1, "%s: ", colored("Opening file for writing", FG_BLUE, COLOR_NONE));
  bprintf(1, "%s ...", YELLOW(BIGFILENAME));

  if ((fd = open(BIGFILENAME, O_CREATE | O_WRONLY)) < 0) {
    bprintf(2, colored("error while opening file\n", FG_RED, COLOR_NONE));
    exit();
  }

  char data = 'a';
  char block[BSIZE], data_integrity = 1;
  uint blocks_written = 0, blocks_read = 0;

  bprintf(1, GREEN("done\nWriting...\n"));

  while (1) {
    memset(block, data, sizeof(block));

    if (write(fd, block, sizeof(block)) <= 0)
      break;

    blocks_written++;
    data = 'a' + (data - 'a' + 1) % 26;

    // printing this, because we get a kernel panic :/
    // it was because there weren't enough blocks
    // on the file system and the maxfile needed more of them
    bprintf(1, "\rBlocks written: %d", blocks_written);
  }

  bprintf(1, "\nWritten ");
  setfgcolor(FG_YELLOW);
  bprintf(1, "%d", blocks_written);
  setfgcolor(COLOR_NONE);
  bprintf(1, " blocks\n");

  bprintf(1, "%s: ", colored("Closing file", FG_BLUE, COLOR_NONE));
  bprintf(1, "%s ...", YELLOW(BIGFILENAME));
  close(fd);
  bprintf(1, GREEN("done\n"));

  bprintf(1, "%s: ", colored("\nOpening file for reading", FG_BLUE, COLOR_NONE));
  bprintf(1, "%s ...", YELLOW(BIGFILENAME));

  if ((fd = open(BIGFILENAME, O_RDONLY)) < 0) {
    bprintf(2, colored("error while opening file\n", FG_RED, COLOR_NONE));
    exit();
  }

  bprintf(1, GREEN("done\nReading...\n"));
  data = 'a';

  while (data_integrity && read(fd, block, sizeof(block)) > 0) {
    for (uint i = 0; i < sizeof(block); i++) {
      if (block[i] != data) {
        bprintf(2, "Corrupted data> block: %d, i: %d, data: %d, block[i]: %d\n",
                blocks_read + 1, i, data, block[i]);

        data_integrity = 0;
        break;
      }
    }

    blocks_read++;
    data = 'a' + (data - 'a' + 1) % 26;

    bprintf(1, "\rBlocks read: %d", blocks_read);
  }

  bprintf(1, "\nRead ");
  setfgcolor(FG_YELLOW);
  bprintf(1, "%d", blocks_read);
  setfgcolor(COLOR_NONE);
  bprintf(1, " blocks\n");

  bprintf(1, "%s: ", colored("Closing file", FG_BLUE, COLOR_NONE));
  bprintf(1, "%s ...", YELLOW(BIGFILENAME));
  close(fd);
  bprintf(1, GREEN("done\n"));

  bprintf(1, "\nblocks_read == blocks_written => %s\n",
          blocks_read == blocks_written ? "true" : "false");

  bprintf(1, "Data integrity: %s\n", data_integrity ? colored("passed", FG_GREEN, COLOR_NONE) : colored("failed", FG_RED, COLOR_NONE));

  bprintf(1, "%s: ", colored("Removing file", FG_BLUE, COLOR_NONE));
  bprintf(1, "%s ...", YELLOW(BIGFILENAME));

  if (unlink(BIGFILENAME) < 0) {
    bprintf(2, colored("couldn't remove file!\n", FG_RED, COLOR_NONE));
  } else {
    bprintf(1, GREEN("done\n"));
  }

  exit();
}
