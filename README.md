# Linux_Processer_interrupts_c

***

# Linux Processer & Interrupts (interrupts.c)

[cite_start]A comprehensive study guide and practical exercise set for understanding Linux processes, software interrupts (signals), and process management in C. This repository contains the `interrupts.c` project, modeled after long-running sensor daemons for embedded systems like the Raspberry Pi Zero 2W. [cite: 511, 513, 633]

## 🎯 Learning Objectives
* [cite_start]Understand the difference between a **Program** (passive file) and a **Process** (active execution). [cite: 549, 550]
* [cite_start]Master **Signal Handling** (Software Interrupts) to communicate with background processes. [cite: 593, 594]
* [cite_start]Explore **Process Creation** using the `fork()` system call. [cite: 561, 562]
* [cite_start]Learn to monitor and control process states (**Running, Sleeping, Stopped, Zombie**) via the Linux terminal. [cite: 588, 589]

---

## 🛠️ Setup & Workflow

### 1. Prerequisites
Ensure you have the necessary build tools installed:
```bash
sudo apt update
sudo apt install build-essential  # For PC/Ubuntu
sudo apt install gcc              # For Raspberry Pi
```
[cite_start][cite: 525, 528, 536]

### 2. Compilation
Compile the source code using `gcc`:
```bash
gcc -o interrupts interrupts.c
```
[cite_start][cite: 521, 669]

### 3. Execution (Two-Terminal Method)
[cite_start]Because the program runs in an infinite loop, you must use two terminal windows: [cite: 541]
* [cite_start]**Terminal 1:** Run the program (`./interrupts`). [cite: 542, 543]
* [cite_start]**Terminal 2:** Send signals to control the program (e.g., `kill -USR1 <PID>`). [cite: 544, 545]

---

## 📡 Signal Reference Table
[cite_start]Signals are the primary interface for interacting with background processes: [cite: 594]

| Signal | Code | Action | Purpose |
| :--- | :--- | :--- | :--- |
| **SIGINT** | (2) | `Ctrl+C` | [cite_start]Request a "graceful" shutdown. [cite: 594] |
| **SIGTERM**| (15) | `kill <pid>` | [cite_start]Default termination signal; can be caught. [cite: 594] |
| **SIGKILL**| (9) | `kill -9 <pid>` | Immediate kernel-level kill; [cite_start]**cannot** be caught. [cite: 594] |
| **SIGSTOP**| (19) | `kill -STOP` | [cite_start]Pauses the process. [cite: 594] |
| **SIGCONT**| (18) | `kill -CONT` | [cite_start]Resumes a paused process. [cite: 594] |
| **SIGUSR1**| (10) | User-defined | [cite_start]In this project: **Resets the counter**. [cite: 594, 651] |
| **SIGUSR2**| (12) | User-defined | [cite_start]In this project: **Reports current stats**. [cite: 594, 473] |
| **SIGHUP** | (1) | Toggle | [cite_start]In this project: **Toggles Verbose mode**. [cite: 594, 479] |

---

## 📂 Project Structure & Exercises

[cite_start]The provided `interrupts.c` serves as a foundation for 9 progressive exercises: [cite: 461, 631, 632]

1.  [cite_start]**SIGFPE Handling:** Catch division-by-zero errors. [cite: 462]
2.  [cite_start]**Recovery:** Use `setjmp()` and `longjmp()` to survive crashes. [cite: 468, 470, 471]
3.  [cite_start]**Custom Stats:** Implement `SIGUSR2` for reporting. [cite: 473]
4.  [cite_start]**Graceful Exit:** Handle `SIGTERM` identically to `SIGINT`. [cite: 476, 478]
5.  [cite_start]**Verbose Toggling:** Use `SIGHUP` to change logging levels without restarting. [cite: 479, 482]
6.  [cite_start]**Software Timers:** Implement `SIGALRM` for a 5-second heartbeat. [cite: 485, 486]
7.  [cite_start]**Signal Blocking:** Use `sigprocmask()` to temporarily ignore signals. [cite: 491, 494]
8.  [cite_start]**Terminal Mastery:** Control the process exclusively through `kill` commands. [cite: 495, 496]
9.  [cite_start]**Forking:** Create a child process and monitor it with `waitpid()`. [cite: 498, 499]

---

## 🔍 Useful Commands for Debugging
* [cite_start]`ps aux | grep interrupts`: Find the Process ID (PID) and status. [cite: 547, 592]
* [cite_start]`pstree -p`: View the process hierarchy tree. [cite: 560]
* `top`: Monitor real-time process activity.
* [cite_start]`echo $$`: Display the PID of your current shell. [cite: 546, 580]

---
[cite_start]*Created as part of the "Programmering av inbyggda system" course at Jensen YH.* [cite: 513]
