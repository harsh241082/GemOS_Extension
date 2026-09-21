// PA2: Mixed workload - spawns CPU-bound and syscall-heavy children, reports MLFQ info
// Run: pa2_tests/mixed [n_cpu] [n_syscall]
// Demonstrates CPU-bound migrate down, syscall-heavy stay up, no starvation

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

static void
cpu_worker(int n)
{
  for (int i = 0; i < n; i++) {
    volatile int x = 0;
    for (int j = 0; j < 100; j++)
      x = x + 1;
  }
  exit(0);
}

static void
syscall_worker(int n)
{
  for (int i = 0; i < n; i++) {
    (void) getsyscount();
    (void) getlevel();
  }
  exit(0);
}

int
main(int argc, char *argv[])
{
  int n_cpu = 2, n_syscall = 2;
  if (argc > 1) n_cpu = atoi(argv[1]);
  if (argc > 2) n_syscall = atoi(argv[2]);
  if (n_cpu <= 0) n_cpu = 2;
  if (n_syscall <= 0) n_syscall = 2;

  printf("[mixed] spawning %d CPU-bound + %d syscall-heavy\n", n_cpu, n_syscall);

  int pids[8];
  int n = 0;

  for (int i = 0; i < n_cpu; i++) {
    int pid = fork();
    if (pid == 0) {
      cpu_worker(50000);
    }
    if (pid > 0)
      pids[n++] = pid;
  }

  for (int i = 0; i < n_syscall; i++) {
    int pid = fork();
    if (pid == 0) {
      syscall_worker(300);
    }
    if (pid > 0)
      pids[n++] = pid;
  }

  // Wait a bit for scheduling to happen
  pause(20);

  printf("\n[mixed] MLFQ info (after ~20 ticks):\n");
  for (int i = 0; i < n; i++) {
    struct mlfqinfo info;
    if (getmlfqinfo(pids[i], &info) == 0) {
      printf("  pid=%d level=%d times_sched=%d syscalls=%d ticks=[%d,%d,%d,%d]\n",
             pids[i], info.level, info.times_scheduled, info.total_syscalls,
             info.ticks[0], info.ticks[1], info.ticks[2], info.ticks[3]);
    }
  }

  for (int i = 0; i < n; i++)
    wait(0);

  printf("[mixed] done\n");
  exit(0);
}
