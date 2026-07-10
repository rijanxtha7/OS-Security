# Task 1 - Test Evidence
**Author:** Rijan Shrestha  
**Date:** July 2026  
**Module:** ST5039CMD - Programming and Operating Systems

---

## Overview

This document provides evidence that the privilege separated authentication
system works correctly. All tests were run on Kali Linux in VirtualBox.

The system consists of two independent processes:
- frontend.c - handles user input, launched with sudo ./frontend
- backend.c - validates credentials, automatically launched via fork()+execve()

---

## Test 1 - Failed Authentication (AUTH_FAIL)

**Input:** Username: hacker, Password: wrongpass

**Terminal Output:**
[] Launching backend process...
[] Backend launched as child process (PID: 2351)
[backend] Starting authentication service...
[backend] Initial UID: 0 (effective: 0)
[backend] Socket bound to /tmp/auth.sock
[backend] Dropping privileges to nobody (UID 65534)...
[backend] Privileges dropped successfully.
[backend] Running as UID: 65534 (effective: 65534)
[backend] Waiting for frontend connection...
Username: hacker
Password:
[] Credentials received for user: hacker
[] Connected to backend at /tmp/auth.sock
[] Sending credentials to backend...
[backend] Connection from PID: 2350, UID: 0
[backend] Frontend connected and verified.
[backend] Received credentials for user: hacker
[backend] Authentication FAILED
[backend] Done. Exiting.
[] Backend response: AUTH_FAIL
[-] Authentication FAILED
[] Backend process finished.
[] Credentials cleared from memory.

**Result:** AUTH_FAIL correctly returned for invalid credentials. ✅

---

## Test 2 - Successful Authentication (AUTH_OK)

**Input:** Username: admin, Password: secret123

**Terminal Output:**
[] Launching backend process...
[] Backend launched as child process (PID: 2613)
[backend] Starting authentication service...
[backend] Initial UID: 0 (effective: 0)
[backend] Socket bound to /tmp/auth.sock
[backend] Dropping privileges to nobody (UID 65534)...
[backend] Privileges dropped successfully.
[backend] Running as UID: 65534 (effective: 65534)
[backend] Waiting for frontend connection...
Username: admin
Password:
[] Credentials received for user: admin
[] Connected to backend at /tmp/auth.sock
[] Sending credentials to backend...
[backend] Connection from PID: 2612, UID: 0
[backend] Frontend connected and verified.
[backend] Received credentials for user: admin
[backend] Authentication SUCCESSFUL
[backend] Done. Exiting.
[] Backend response: AUTH_OK
[+] Authentication SUCCESSFUL
[] Backend process finished.
[] Credentials cleared from memory.

**Result:** AUTH_OK correctly returned for valid credentials. ✅

---

## Test 3 - Privilege Drop Verification

Both tests above confirm privilege dropping worked correctly:

- Backend started as UID 0 (root)
- After binding socket, called setresuid(65534, 65534, 65534)
- Verified with geteuid() - confirmed UID 65534 (nobody)
- Backend validated credentials while running as unprivileged user

**Evidence from output:**
[backend] Initial UID: 0 (effective: 0)       <- started as root
[backend] Dropping privileges to nobody (UID 65534)...
[backend] Privileges dropped successfully.
[backend] Running as UID: 65534 (effective: 65534)  <- dropped to nobody

This proves setresuid() was called and verified with geteuid(). ✅

---

## Test 4 - SO_PEERCRED Peer Verification

Both tests confirm peer verification worked:
[backend] Connection from PID: 2612, UID: 0
[backend] Frontend connected and verified.

Backend checked the connecting process UID using SO_PEERCRED
and accepted only trusted connections. ✅

---

## Summary

| Test | Input | Expected | Result |
|------|-------|----------|--------|
| Wrong credentials | hacker/wrongpass | AUTH_FAIL | ✅ PASS |
| Correct credentials | admin/secret123 | AUTH_OK | ✅ PASS |
| Privilege drop | setresuid() | UID 65534 | ✅ PASS |
| Peer verification | SO_PEERCRED | Verified | ✅ PASS |

All Task 1 requirements verified and working correctly.
