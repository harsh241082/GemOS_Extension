#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

int
main(int argc, char *argv[])
{
  printf("=== Part B: Process Relationships ===\n");

  // Test B1: getppid()
  printf("\nTest B1: getppid() syscall\n");
  int ppid = getppid();
  printf("getppid() returned: %d\n", ppid);
  printf("Current process ID: %d\n", getpid());
  if(ppid > 0) {
    printf("✓ Parent process ID is valid: %d\n", ppid);
  } else {
    printf("✗ Parent process ID is invalid\n");
  }

  // Test B2: getnumchild() with fork
  printf("\nTest B2: getnumchild() syscall\n");
  int num_children = getnumchild();
  printf("Before fork: getnumchild() = %d\n", num_children);

  int pid1 = fork();
  if(pid1 == 0) {
    // Child 1
    printf("Child 1 (PID %d): Parent is %d\n", getpid(), getppid());
    exit(0);
  } else if(pid1 > 0) {
    // Parent: fork another child
    int pid2 = fork();
    if(pid2 == 0) {
      // Child 2
      printf("Child 2 (PID %d): Parent is %d\n", getpid(), getppid());
      exit(0);
    } else if(pid2 > 0) {
      // Parent: check child count
      num_children = getnumchild();
      printf("After 2 forks: getnumchild() = %d\n", num_children);
      if(num_children == 2) {
        printf("✓ Correctly counted 2 children\n");
      } else {
        printf("✗ Expected 2 children, got %d\n", num_children);
      }

      // Wait for children
      wait((int*)0);
      wait((int*)0);

      num_children = getnumchild();
      printf("After children exit: getnumchild() = %d\n", num_children);
      if(num_children == 0) {
        printf("✓ No zombie children counted\n");
      } else {
        printf("✗ Still counted %d children\n", num_children);
      }
    }
  }

  printf("\n=== Part B Tests Complete ===\n");
  exit(0);
}
