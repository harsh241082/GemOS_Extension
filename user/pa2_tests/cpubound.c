// PA2: CPU-bound process - minimal syscalls, should demote to lower queues
// Run: pa2_tests/cpubound [iterations]
// Uses busy loop; ΔS < ΔT so process will be demoted when time slice expires

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int n = 100000;
  if (argc > 1)
    n = atoi(argv[1]);
  if (n <= 0)
    n = 100000;

  int pid = getpid();
  printf("[cpubound] pid=%d starting, n=%d\n", pid, n);

  for (int i = 0; i < n; i++) {
    // Pure CPU work - no syscalls in the loop
    volatile int x = 0;
    for (int j = 0; j < 100; j++)
      x = x + 1;
  }

  int level = getlevel();
  struct mlfqinfo info;
  if (getmlfqinfo(pid, &info) == 0) {
    printf("[cpubound] pid=%d done: level=%d times_sched=%d syscalls=%d ticks=[%d,%d,%d,%d]\n",
           pid, info.level, info.times_scheduled, info.total_syscalls,
           info.ticks[0], info.ticks[1], info.ticks[2], info.ticks[3]);
  } else {
    printf("[cpubound] pid=%d done: level=%d\n", pid, level);
  }

  exit(0);
}
