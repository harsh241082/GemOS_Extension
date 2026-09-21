// PA2: Test global priority boost - run CPU-bound long enough to demote,
// then wait for boost (every 128 ticks) and verify level resets to 0
// Run: pa2_tests/boost
// Prints level at start, after work, and after waiting for boost

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int pid = getpid();
  uint t0 = uptime();

  printf("[boost] pid=%d start: level=%d uptime=%d\n", pid, getlevel(), t0);

  // CPU-bound work to demote
  for (int i = 0; i < 100000; i++) {
    volatile int x = 0;
    for (int j = 0; j < 100; j++)
      x = x + 1;
  }

  int level_before = getlevel();
  uint t1 = uptime();
  printf("[boost] after work: level=%d uptime=%d\n", level_before, t1);

  // Wait until we cross a boost boundary (every 128 ticks)
  uint target = ((t1 / 128) + 1) * 128;
  if (target > t1 + 10)
    pause(target - t1);

  int level_after = getlevel();
  uint t2 = uptime();
  printf("[boost] after boost wait: level=%d uptime=%d (boost every 128)\n", level_after, t2);

  struct mlfqinfo info;
  if (getmlfqinfo(pid, &info) == 0) {
    printf("[boost] final: level=%d ticks=[%d,%d,%d,%d]\n",
           info.level, info.ticks[0], info.ticks[1], info.ticks[2], info.ticks[3]);
  }

  exit(0);
}
