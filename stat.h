#ifndef STAT_H
#define STAT_H

#include "types.h"

#define T_DIR 1  // Directory
#define T_FILE 2 // File
#define T_DEV 3  // Device

// mid-term project
#define T_SYMLINK 4 // Symlink

// for `stat` system call
// to not dereference the symlink
#define S_NOFOLLOW 0x004

struct stat {
  short type;  // Type of file
  int dev;     // File system's disk device
  uint ino;    // Inode number
  short nlink; // Number of links to file
  uint size;   // Size of file in bytes
};

#endif // STAT_H
