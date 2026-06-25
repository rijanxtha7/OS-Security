# Task 1 Research Notes - Privilege Separation
**Author:** Rijan Shrestha  
**Date:** June 2026

## 1. Process Isolation

Process isolation is when different parts of a program run as completely separate
OS processes. Each process has its own memory space, file descriptors, and privileges.
If one process is compromised, the attacker still cannot directly access the other
process's memory or credentials.

In Task 1, the frontend process is used to receive input from the user and the
backend process handles password validation for sensitive passwords. They are
deliberately kept separate so that even if the frontend is exploited, the attacker
cannot access the backend directly or read the password database.

Reference: Tanenbaum, A.S. (2015) Modern Operating Systems, 4th ed.

## 2. fork() and execve()

fork() is a system call in Linux which creates a new child process that is a copy
of the parent process. Both parent and child run from the same starting point in
the code, but fork() returns different values - 0 to the child, and the child PID
to the parent.

execve() swaps the current image of the process with a new program. When the child
calls execve("./backend", ...), the child stops running the parent code and instead
loads and executes backend.c. This is how two separate executables are launched
from one program.

The combination of fork() and execve() is known as the fork-exec pattern and is
the standard Unix way of launching a new program.

Reference: Kerrisk, M. (2010) The Linux Programming Interface. No Starch Press.

## 3. setresuid() - Privilege Dropping

setresuid() changes the real, effective and saved user IDs of a process
simultaneously. When a backend process starts with root privileges to bind to a
socket, it must immediately drop those privileges after completing that operation.

By calling setresuid(65534, 65534, 65534) - the UID of "nobody" - the backend
permanently gives up root access. The saved UID is also updated, making it
impossible for the process to regain root access even if attacked.

This implements the Principle of Least Privilege: a process should only have the
least privileges necessary to perform its task.

Reference: https://man7.org/linux/man-pages/man2/setresuid.2.html

## 4. UNIX Domain Sockets

UNIX domain sockets are an inter-process communication (IPC) mechanism that
allows two processes on the same machine to exchange data through a file path
(e.g. /tmp/auth.sock).

Unlike network sockets (TCP/IP), UNIX domain sockets:
- Never leave the machine - no network exposure
- Support SO_PEERCRED - allowing a process to verify the UID/PID of
  the connecting process
- Are faster because they do not go through the network stack
- Can only be accessed by processes that have appropriate permission to
  use the socket file

For authentication systems, UNIX domain sockets are preferred since
credentials never travel over any network interface.

Reference: Stevens, W.R. (2003) Unix Network Programming, Vol 1. Prentice Hall.

## 5. Principle of Least Privilege

The principle of least privilege states that each process, user or program should
only be granted the minimum access needed to accomplish its task.

In Task 1 this is upheld by:
- Frontend: never sees the password data, only handles user input
- Backend: starts with elevated privileges to bind the socket, then drops
  to nobody immediately after
- IPC: credentials are sent only through a controlled socket channel

Reference: Saltzer, J.H. and Schroeder, M.D. (1975) The Protection of
Information in Computer Systems. Proceedings of the IEEE.

## 6. Secure Memory Clearing

After the frontend sends a password over the socket, the password stays in
memory until it is overwritten. A standard memset() call can be optimised
away by the compiler if it detects the buffer is never read again.

explicit_bzero() or volatile memset() force the memory wipe to actually
execute, ensuring passwords are not left in memory after use.

Reference: https://man7.org/linux/man-pages/man3/explicit_bzero.3.html
