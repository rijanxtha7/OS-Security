# Task 2 Research Notes - Malware Analysis Sandbox
**Author:** Rijan Shrestha  
**Date:** June 2026

## 1. Parent-Child Process Isolation

fork() can be used to create a child process which is identical to the parent process
and runs independently. If the child is used to run an unknown binary using
execve(), any malicious behaviour from that binary is contained within
only the child process.

The parent process monitors from the outside. It can track, regulate,
and forcibly terminate the child without being affected by whatever the
child does. This is the basic idea behind process level sandboxing.

This is more secure than executing untrusted code in the same process
because separate processes have different memory spaces, separate file
descriptors, and separate privilege contexts.

Reference: Tanenbaum, A.S. (2015) Modern Operating Systems, 4th ed.

## 2. pthreads - POSIX Threads

pthreads allow multiple tasks to run concurrently within a single process.
In the sandbox, the parent process needs to monitor two things at the same
time - execution time and CPU usage. Creating separate threads for each
monitoring task allows them to run at the same time.

pthread_create() creates a new thread, pthread_join() waits for it to
complete, and pthread_mutex_t provides locking to prevent threads from
corrupting shared data.

Without threads, the parent would need to check time and CPU usage one
at a time, which could allow a malicious process to slip through the
monitoring window.

Reference: Kerrisk, M. (2010) The Linux Programming Interface. No Starch Press.

## 3. Signals - SIGKILL and SIGTERM

Signals are asynchronous notifications sent from the OS or another process
to a target process. They are the main way to enforce termination
policies in a sandbox.

SIGTERM (signal 15) politely asks a process to terminate. The process
can catch this signal and clean up before exiting. However an untrusted
binary may deliberately ignore SIGTERM.

SIGKILL (signal 9) cannot be caught, blocked or ignored by the receiving
process. It is handled directly by the OS kernel and immediately kills the
process. This makes SIGKILL a reliable enforcement tool for a sandbox
where the monitored process cannot be trusted to cooperate.

kill(child_pid, SIGKILL) is called by the parent when the child exceeds
the allowed execution time or CPU usage.

Reference: https://man7.org/linux/man-pages/man7/signal.7.html

## 4. /proc Filesystem - External Monitoring

The /proc filesystem is a virtual filesystem in Linux that exposes
real-time information about running processes. Every process has a
directory at /proc/[pid]/ containing files with live data.

/proc/[pid]/stat holds CPU time information including utime (user mode
CPU time) and stime (kernel mode CPU time). By reading and parsing
this file, the parent process can measure how much CPU the child is
consuming without any cooperation from the child.

This external monitoring approach is important for sandbox security
because the monitored process cannot modify or hide its own /proc
entries from the supervising parent.

Reference: https://man7.org/linux/man-pages/man5/proc.5.html

## 5. Race Conditions and Mutex Locks

A race condition happens when two or more threads access and modify shared
data simultaneously, producing unpredictable results. In a sandbox with
multiple monitoring threads updating a shared kill_flag variable, a race
condition could cause the flag to be set incorrectly or missed entirely.

pthread_mutex_t (mutex lock) ensures only one thread can access the
shared variable at a time. A thread locks the mutex before reading or
writing, then unlocks it after. Other threads must wait until the
mutex is released.

This ensures the kill decision is made correctly even when multiple
monitoring threads detect problems at the same time.

Reference: Kerrisk, M. (2010) The Linux Programming Interface. No Starch Press.

## 6. Atomic Operations

Atomic operations are performed in a single uninterruptible step. In C,
the C11 stdatomic.h library provides atomic types like atomic_bool which
ensure that reads and writes to a variable are thread-safe without
needing a full mutex lock.

Using volatile bool instead of atomic_bool is not enough because the
C language memory model does not guarantee that changes made by one thread
are immediately visible to another thread. Atomic operations include
the necessary memory barriers to ensure visibility across threads.

In the sandbox, atomic_bool is used for the kill_flag so all monitoring
threads see the correct value immediately when termination is triggered.

Reference: ISO/IEC 9899:2011 - C11 Standard, Section 7.17 Atomics.
