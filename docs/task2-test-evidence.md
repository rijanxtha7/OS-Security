# Task 2 - Test Evidence
**Author:** Rijan Shrestha  
**Date:** July 2026  
**Module:** ST5039CMD - Programming and Operating Systems

---

## Overview

This document provides evidence that the malware analysis sandbox
works correctly. All tests were run on Kali Linux in VirtualBox.

The sandbox consists of:
- sandbox.c - parent supervisor process
- test_normal.c - simulates a normal well-behaved program
- test_infinite.c - simulates malware that loops forever
- test_memory.c - simulates malware that allocates huge memory

Enforcement mechanisms:
- Time limit: 5 seconds (SIGKILL via timer thread)
- Memory limit: 50MB (RLIMIT_AS via setrlimit)
- CPU monitoring: /proc/[pid]/stat read every second

---

## Test 1 - Normal Binary (Clean Exit)

Command: ./sandbox ./test_normal
Expected: Child exits cleanly within time limit

Terminal Output:
[21:39:25] [INFO] Starting sandbox...
[21:39:25] [INFO] Target binary: ./test_normal
[21:39:25] [INFO] Limits: 5s time, 50MB memory
[21:39:25] [SANDBOX] Child process created with PID: 2597
[21:39:25] [CHILD] Memory limit set to 50MB
[21:39:25] [TIMER] Timer thread started. Limit: 5 seconds
[21:39:25] [MONITOR] CPU monitor thread started.
[21:39:25] [MONITOR] Child PID 2597 - CPU ticks: user=0 kernel=0
[test_normal] Starting...
[test_normal] Doing some work...
[21:39:26] [MONITOR] Child PID 2597 - CPU ticks: user=0 kernel=0
[test_normal] Done. Exiting normally.
[21:39:27] [MONITOR] CPU monitor thread exiting.
[21:39:30] [SANDBOX] Child exited normally with code: 0
[21:39:30] [INFO] Sandbox session complete.

Result: Child exited normally with code 0.
Observation: Timer thread was cancelled before triggering.
CPU usage remained at 0 ticks - normal for a simple program.

---

## Test 2 - Infinite Loop Binary (Timeout Enforcement)

Command: ./sandbox ./test_infinite
Expected: Child killed by SIGKILL after 5 seconds

Terminal Output:
[21:39:38] [INFO] Starting sandbox...
[21:39:38] [INFO] Target binary: ./test_infinite
[21:39:38] [INFO] Limits: 5s time, 50MB memory
[21:39:38] [SANDBOX] Child process created with PID: 2710
[21:39:38] [CHILD] Memory limit set to 50MB
[21:39:38] [TIMER] Timer thread started. Limit: 5 seconds
[21:39:38] [MONITOR] CPU monitor thread started.
[21:39:38] [MONITOR] Child PID 2710 - CPU ticks: user=0 kernel=0
[test_infinite] Starting infinite loop...
[test_infinite] This program will never exit!
[21:39:39] [MONITOR] Child PID 2710 - CPU ticks: user=104 kernel=0
[21:39:40] [MONITOR] Child PID 2710 - CPU ticks: user=216 kernel=0
[21:39:41] [MONITOR] Child PID 2710 - CPU ticks: user=316 kernel=0
[21:39:42] [MONITOR] Child PID 2710 - CPU ticks: user=416 kernel=0
[21:39:43] [TIMER] Time limit exceeded! Killing child PID: 2710
[21:39:43] [TIMER] SIGKILL sent to child process.
[21:39:43] [MONITOR] CPU monitor thread exiting.
[21:39:43] [SANDBOX] Child was killed by signal: 9 (Killed)
[21:39:43] [INFO] Sandbox session complete.

Result: Child killed by SIGKILL after exactly 5 seconds.
Observation: CPU ticks increased every second proving the
infinite loop was consuming CPU. The child had no way to prevent
or delay the SIGKILL from the parent supervisor.

---

## Test 3 - Memory Hog Binary (Memory Limit Enforcement)

Command: ./sandbox ./test_memory
Expected: Child stopped at 50MB memory limit by RLIMIT_AS

Terminal Output:
[21:39:56] [INFO] Starting sandbox...
[21:39:56] [INFO] Target binary: ./test_memory
[21:39:56] [INFO] Limits: 5s time, 50MB memory
[21:39:56] [SANDBOX] Child process created with PID: 2855
[21:39:56] [CHILD] Memory limit set to 50MB
[21:39:56] [MONITOR] CPU monitor thread started.
[21:39:56] [TIMER] Timer thread started. Limit: 5 seconds
[21:39:56] [MONITOR] Child PID 2855 - CPU ticks: user=0 kernel=0
[test_memory] Starting memory allocation attack...
[test_memory] Allocated 10MB so far...
[test_memory] Allocated 20MB so far...
[test_memory] Allocated 30MB so far...
[test_memory] Allocated 40MB so far...
[test_memory] malloc failed at iteration 4
[21:39:57] [MONITOR] CPU monitor thread exiting.
[21:40:01] [SANDBOX] Child exited normally with code: 0
[21:40:01] [INFO] Sandbox session complete.

Result: Memory allocation stopped at 40MB due to 50MB RLIMIT_AS.
Observation: The child process could not allocate beyond the
limit set by the parent using setrlimit(RLIMIT_AS). The child
had no way to remove or bypass this limit.

---

## Summary

Test 1 - test_normal  - No limit triggered - Clean exit
Test 2 - test_infinite - Time limit 5s     - SIGKILL sent
Test 3 - test_memory   - Memory limit 50MB - malloc failed

## Key Observations

1. The untrusted binary had no control over its own termination
2. CPU usage was monitored externally via /proc/[pid]/stat
3. SIGKILL cannot be caught or ignored by the child process
4. RLIMIT_AS enforced memory limit before child could exceed 50MB
5. atomic_bool kill_flag prevented race conditions between threads
6. Two monitoring threads ran concurrently using pthreads
