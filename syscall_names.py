#!/usr/bin/python3

import re

rgx = re.compile(r"(?<=SYS_)\w+")

syscalls = []

with open("syscall.h", "r") as file:
    for ln, line in enumerate(file.readlines()):
        result = rgx.findall(line)

        if len(result) == 1:
            syscalls.append(result[0])

with open("syscall_names.c", "w") as file:
    file.write("// this is an autogenerated file - created by syscall_names.py\n")

    file.write(
        "\n".join(
            [
                '#include "syscall.h"',

                "\n// syscall counts",
                f"const int NUM_SYSCALLS = {len(syscalls)};",

                "\nchar *syscall_names[] = {",
                ",\n".join([f'  [SYS_{syscall}] "{syscall}"' for syscall in syscalls]),
                "};",

                "\n// syscall function prototypes",
                "\n".join([f'extern int sys_{syscall}(void);' for syscall in syscalls]),

                "\n// syscalls array",
                "int (*syscalls[])(void) = {",
                ",\n".join([f'  [SYS_{syscall}] sys_{syscall}' for syscall in syscalls]),
                "};",

                "\n"
            ]
        )
    )
