# C Shell

A lightweight Unix-like shell written in C with a modular architecture and POSIX-compliant process handling. This project was built to demonstrate core shell concepts such as parsing user input, executing commands, handling pipes and redirection, and managing background jobs.

## Overview

This project implements a custom Unix-like shell in C that mimics core shell behavior through a modular architecture. It focuses on the fundamentals of command parsing, process execution, filesystem navigation, and job control. The shell is designed to be interactive, responsive, and suitable for demonstrating systems programming concepts in a resume or portfolio project.

The implementation covers several major parts of the assignment, including shell input handling, parsing, built-in commands, file redirection, pipelines, and background job management. Each component is separated into its own source and header files to keep the project organized and easy to understand.

The shell supports both built-in commands and external program execution, allowing it to behave like a lightweight terminal while still being simple enough to study and extend. This makes it a strong example of low-level C programming and operating system concepts.

## Features

### Shell Interface
- Displays a clean prompt in the form `[user@host:directory]$` so the shell feels interactive and user-friendly.
- Tracks the shell’s home directory and current working directory so navigation and path display behave naturally.
- Supports directory shortcuts such as `~`, `.`, `..`, and `-`, making the shell more convenient to use.

### Command Handling
- Parses user input and validates basic shell syntax before execution, helping catch malformed commands early.
- Executes external programs using `execvp`, allowing the shell to run standard Unix commands such as `cat`, `echo`, and `sleep`.
- Supports input and output redirection with `<`, `>`, and `>>`, enabling file-based I/O behavior similar to a real shell.
- Supports command chaining with `|`, `;`, and `&`, making it possible to build pipelines and manage sequential or background work.

### Built-in Commands
- `hop` changes the current working directory and supports both relative and absolute paths, including special directory shortcuts.
- `reveal` lists the contents of a directory and can be used to view files and folders in a simple, shell-like way.
- `log` stores recent commands in persistent history, allowing users to review, clear, or re-execute earlier input.
- `activities` shows currently active jobs and their states, which helps users monitor running background processes.
- `ping` sends signals to processes by PID, demonstrating low-level process control through the Unix signal interface.
- `fg` and `bg` move jobs between the foreground and background, giving the shell basic job control capabilities.

### Job Control
- Launches background processes and reports their job IDs and PIDs so users can track them easily.
- Handles stopped and resumed jobs, allowing the shell to pause execution and continue it later.
- Responds to basic shell signals such as Ctrl-C and Ctrl-Z, showing how interactive shells manage foreground processes.

## Project Structure

```text
C-Shell/
├── Makefile
├── README.md
├── include/
│   ├── bg.h
│   ├── execute.h
│   ├── hop.h
│   ├── input.h
│   ├── log.h
│   ├── parser.h
│   ├── prompt.h
│   ├── reveal.h
│   └── signals.h
│   └── types.h
├── src/
│   ├── bg.c
│   ├── execute.c
│   ├── hop.c
│   ├── input.c
│   ├── log.c
│   ├── main.c
│   ├── parser.c
│   ├── prompt.c
│   ├── reveal.c
│   └── signals.c
└── obj/
```

## Build and Run

Build and launch the shell with:

```bash
make all
```

This compiles the project and produces the executable `shell.out`, then runs it.

If you want to just build it without launching:

```bash
make
```

To clean the build artifacts:

```bash
make clean
```

## Tech Stack

- Language: C
- Standard: C99 with POSIX support
- Libraries: `unistd.h`, `sys/wait.h`, `fcntl.h`, `signal.h`, `dirent.h`, and related POSIX APIs

## Why This Project Matters

This project demonstrates solid systems programming fundamentals, including process creation, inter-process communication, filesystem interaction, signal handling, and modular software design. It is a strong example of low-level C programming and shell internals.

## Author

Built as a personal systems programming project focused on understanding how Unix shells works.