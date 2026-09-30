# Ferry Crossing

> **Course homework: BIL461 - HW3.** This repository contains an assignment submission, not a production library.

A C program that simulates a ferry carrying cars across a river, using POSIX threads, semaphores and a mutex for synchronization.

## How It Works

- **Ferry thread:** Hands out boarding tickets for a fixed number of cars (capacity 5), waits until the ferry is full, crosses (3 seconds), lets the cars unload, and returns to the dock.
- **Car threads:** A generator thread keeps creating cars while the ferry is at the dock. Each car waits for a ticket, boards, and unloads after the crossing.
- **Synchronization:** Four semaphores coordinate the steps (board, full, unboard, empty) and a mutex protects the shared car counter.
- **Logging:** Every event is printed with a timestamp. The simulation runs for 60 seconds.

## Repository Structure

```
.
├── ferry_cross.c   simulation source
├── Makefile        build rules
└── README.md
```

## Usage

```bash
make
./ferry_cross
make clean
```

Requires a POSIX system with gcc and pthread support (Linux, macOS, or WSL).
