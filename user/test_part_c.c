#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  printf("=== Part C: System Call Accounting ===\n");

  // Test C1 & C2: getsyscount()
  printf("\nTest C1 & C2: getsyscount() syscall\n");
  int count1 = getsyscount();
  printf("Initial syscall count: %d\n", count1);

  // Make some syscalls
  getpid();
  getpid2();
  getppid();
  
  int count2 = getsyscount();
  printf("After 3 syscalls (getpid, getpid2, getppid): %d\n", count2);
  printf("Count increased by: %d\n", count2 - count1);
  
  if(count2 > count1) {
    printf("✓ System call counter is incrementing\n");
  } else {
    printf("✗ System call counter not incrementing properly\n");
  }

  // Test C3: getchildsyscount()
  printf("\nTest C3: getchildsyscount() syscall\n");
  
  int child_pid = fork();
  if(child_pid == 0) {
    // Child process
    int child_count1 = getsyscount();
    printf("Child: initial syscall count: %d\n", child_count1);
    
    // Make some syscalls in child
    getpid();
    getppid();
    getpid2();
    
    int child_count2 = getsyscount();
    printf("Child: after 3 syscalls, count: %d\n", child_count2);
    
    exit(0);
  } else if(child_pid > 0) {
    // Parent process
    // Query child before it exits
    printf("Parent: spawned child with PID %d\n", child_pid);
    
    // Give child a moment to make syscalls
    int result = getchildsyscount(child_pid);
    printf("Parent: getchildsyscount(%d) before child exit returned: %d\n", child_pid, result);
    
    wait((int*)0);
    
    // After wait, child is reaped and not in process table anymore
    result = getchildsyscount(child_pid);
    printf("Parent: getchildsyscount(%d) after child exit returned: %d\n", child_pid, result);
    
    // Test error case: invalid child PID
    int invalid = getchildsyscount(9999);
    printf("Parent: getchildsyscount(9999) returned: %d\n", invalid);
    if(invalid == -1) {
      printf("✓ getchildsyscount() correctly returned -1 for invalid PID\n");
    } else {
      printf("✗ getchildsyscount() should return -1 for invalid PID\n");
    }
  }

  printf("\n=== Part C Tests Complete ===\n");
  exit(0);
}
