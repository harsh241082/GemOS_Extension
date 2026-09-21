// PA2: Syscall-heavy process - many syscalls per tick, should stay at higher queues
// Run: pa2_tests/syscall [iterations]
// ΔS >= ΔT (interactive) so process should NOT be demoted

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int n = 500;
  if (argc > 1)
    n = atoi(argv[1]);
  if (n <= 0)
    n = 500;

  int pid = getpid();
  printf("[syscall] pid=%d starting, n=%d\n", pid, n);

  for (int i = 0; i < n; i++) {
    // Many syscalls per iteration - getsyscount, getlevel, write
    (void) getsyscount();
    (void) getlevel();
    if (i % 50 == 0)
      write(1, ".", 1);
  }
  write(1, "\n", 1);

  int level = getlevel();
  struct mlfqinfo info;
  if (getmlfqinfo(pid, &info) == 0) {
    printf("[syscall] pid=%d done: level=%d times_sched=%d syscalls=%d ticks=[%d,%d,%d,%d]\n",
           pid, info.level, info.times_scheduled, info.total_syscalls,
           info.ticks[0], info.ticks[1], info.ticks[2], info.ticks[3]);
  } else {
    printf("[syscall] pid=%d done: level=%d\n", pid, level);
  }

  exit(0);
}
