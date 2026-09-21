#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  printf("=== Part A: Warm-up System Calls ===\n");

  // Test A1: hello()
  printf("\nTest A1: hello() syscall\n");
  printf("Calling hello()...\n");
  hello();
  printf("hello() returned successfully\n");

  // Test A2: getpid2()
  printf("\nTest A2: getpid2() syscall\n");
  int pid = getpid2();
  int pid_orig = getpid();
  printf("getpid2() returned: %d\n", pid);
  printf("getpid() returned: %d\n", pid_orig);
  if(pid == pid_orig) {
    printf("✓ getpid2() matches getpid()\n");
  } else {
    printf("✗ getpid2() does not match getpid()\n");
  }

  printf("\n=== Part A Tests Complete ===\n");
  exit(0);
}
