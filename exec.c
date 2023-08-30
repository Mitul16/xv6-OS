#include "defs.h"
#include "elf.h"
#include "memlayout.h"
#include "mmu.h"
#include "param.h"
#include "proc.h"
#include "stat.h"
#include "types.h"
#include "x86.h"

#include "colors.h"

// similar to shell env PATH
// this should be done by `sh`
const char *paths[] = {
    "/", "/bin/",
    0, // for current path
};

const int n_paths = NELEM(paths);

int exec(char *path, char **argv) {
  char *s, *last;
  int i, off;
  uint argc, sz, sp, ustack[3 + MAXARG + 1];
  struct elfhdr elf;
  struct inode *ip;
  struct proghdr ph;
  pde_t *pgdir, *oldpgdir;
  struct proc *curproc = myproc();

  // extra
  int path_found = 0;
  char completed_path[128];
  struct stat st;

  begin_op();

  for (int i = 0; i < n_paths; i++) {
    memset(completed_path, 0, sizeof(completed_path));

    if (paths[i] != 0)
      strncpy(completed_path, paths[i], sizeof(completed_path));

    strncat(completed_path, path, sizeof(completed_path));

    if ((ip = namei(completed_path, 1)) != 0) {
      ilock(ip);
      stati(ip, &st);
      iunlock(ip);

      path_found = 1;
      break;
    }
  }

  // Save program name for debugging.
  for (last = s = path; *s; s++)
    if (*s == '/')
      last = s + 1;

  if (!path_found) {
    end_op();
    cprintf(RED("exec: fail, file not found\n"));
    return -1;
  } else if (st.type != T_FILE) {
    end_op();
    cprintf("exec: %s is not a file\n", last);
    return -1;
  }

  ilock(ip);
  pgdir = 0;

  // Check ELF header
  if (readi(ip, (char *)&elf, 0, sizeof(elf)) != sizeof(elf))
    goto bad;

  if (elf.magic != ELF_MAGIC)
    goto bad;

  if ((pgdir = setupkvm()) == 0)
    goto bad;

  // Load program into memory.
  sz = 0;
  for (i = 0, off = elf.phoff; i < elf.phnum; i++, off += sizeof(ph)) {
    if (readi(ip, (char *)&ph, off, sizeof(ph)) != sizeof(ph))
      goto bad;

    if (ph.type != ELF_PROG_LOAD)
      continue;

    if (ph.memsz < ph.filesz)
      goto bad;

    if (ph.vaddr + ph.memsz < ph.vaddr)
      goto bad;

    if ((sz = allocuvm(pgdir, sz, ph.vaddr + ph.memsz)) == 0)
      goto bad;

    if (ph.vaddr % PGSIZE != 0)
      goto bad;

    if (loaduvm(pgdir, (char *)ph.vaddr, ip, ph.off, ph.filesz) < 0)
      goto bad;
  }

  iunlockput(ip);
  end_op();
  ip = 0;

  // Allocate two pages at the next page boundary.
  // Make the first inaccessible.  Use the second as the user stack.
  sz = PGROUNDUP(sz);
  if ((sz = allocuvm(pgdir, sz, sz + 2 * PGSIZE)) == 0)
    goto bad;

  clearpteu(pgdir, (char *)(sz - 2 * PGSIZE));
  sp = sz;

  // Push argument strings, prepare rest of stack in ustack.
  for (argc = 0; argv[argc]; argc++) {
    if (argc >= MAXARG)
      goto bad;

    sp = (sp - (strlen(argv[argc]) + 1)) & ~3;
    if (copyout(pgdir, sp, argv[argc], strlen(argv[argc]) + 1) < 0)
      goto bad;

    ustack[3 + argc] = sp;
  }

  ustack[3 + argc] = 0;
  ustack[0] = 0xffffffff; // fake return PC
  ustack[1] = argc;
  ustack[2] = sp - (argc + 1) * 4; // argv pointer

  sp -= (3 + argc + 1) * 4;

  if (copyout(pgdir, sp, ustack, (3 + argc + 1) * 4) < 0)
    goto bad;

  safestrcpy(curproc->name, last, sizeof(curproc->name));

  // Commit to the user image.
  oldpgdir = curproc->pgdir;
  curproc->pgdir = pgdir;
  curproc->sz = sz;
  curproc->tf->eip = elf.entry; // main
  curproc->tf->esp = sp;

  switchuvm(curproc);
  freevm(oldpgdir);
  return 0;

bad:
  if (pgdir)
    freevm(pgdir);

  if (ip) {
    iunlockput(ip);
    end_op();
  }

  return -1;
}
