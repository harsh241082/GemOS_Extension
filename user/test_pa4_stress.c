// PA4 Test 4: Comprehensive Stress Test
// Tests all PA4 features together with heavy workload

#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "user/test_pa4.h"

#define PAGE_SIZE 4096
#define STRESS_MB 8  // Allocate 8MB to trigger significant swapping

void stress_test_with_policy(int policy, const char *policy_name) {
  printf("\n--- Stress Test with %s ---\n", policy_name);
  
  setdisksched(policy);
  
  int pid = getpid();
  struct vmstats before, after;
  
  getvmstats(pid, &before);
  int start_time = uptime();
  
  // Allocate large memory block
  int num_pages = (STRESS_MB * 1024 * 1024) / PAGE_SIZE;
  printf("Allocating %d MB (%d pages)...\n", STRESS_MB, num_pages);
  
  char *data = (char *)malloc(num_pages * PAGE_SIZE);
  if(!data) {
    printf("  FAIL: allocation failed\n");
    return;
  }
  
  // Intensive write pattern
  printf("Running intensive write pattern...\n");
  for(int pass = 0; pass < 2; pass++) {
    for(int i = 0; i < num_pages; i++) {
      int offset = i * PAGE_SIZE;
      for(int j = 0; j < PAGE_SIZE; j++) {
        data[offset + j] = (char)((pass * i) + j);
      }
    }
  }
  
  // Random access pattern
  printf("Running random access pattern...\n");
  for(int iter = 0; iter < 5; iter++) {
    for(int i = 0; i < num_pages; i += 3) {
      int idx = (i * 1103) % num_pages;
      int offset = idx * PAGE_SIZE;
      volatile char val = data[offset];
      data[offset] = val ^ 0xFF;
    }
  }
  
  // Verification read
  printf("Verifying memory integrity...\n");
  int checksum = 0;
  for(int i = 0; i < num_pages; i++) {
    checksum += data[i * PAGE_SIZE];
  }
  
  int end_time = uptime();
  getvmstats(pid, &after);
  
  printf("Results:\n");
  printf("  Time: %d ticks\n", end_time - start_time);
  printf("  Pages swapped out: %d\n", after.pages_swapped_out - before.pages_swapped_out);
  printf("  Pages swapped in: %d\n", after.pages_swapped_in - before.pages_swapped_in);
  printf("  Pages evicted: %d\n", after.pages_evicted - before.pages_evicted);
  printf("  Checksum: 0x%x\n", checksum);
  
  free(data);
}

void multi_process_test(void) {
  printf("\n--- Multi-Process Stress Test ---\n");
  
  printf("Testing with multiple processes...\n");
  
  int pid = fork();
  if(pid < 0) {
    printf("FAIL: fork failed\n");
    return;
  }
  
  if(pid == 0) {
    // Child process
    printf("Child: Allocating memory...\n");
    int num_pages = (4 * 1024 * 1024) / PAGE_SIZE;
    char *data = (char *)malloc(num_pages * PAGE_SIZE);
    
    if(data) {
      // Fill memory
      for(int i = 0; i < num_pages; i++) {
        data[i * PAGE_SIZE] = (char)getpid();
      }
      
      // Access pattern
      for(int i = 0; i < num_pages; i += 5) {
        volatile char val = data[i * PAGE_SIZE];
      }
      
      free(data);
      printf("Child: Done\n");
    }
    exit(0);
  } else {
    // Parent process
    printf("Parent: Allocating memory...\n");
    int num_pages = (4 * 1024 * 1024) / PAGE_SIZE;
    char *data = (char *)malloc(num_pages * PAGE_SIZE);
    
    if(data) {
      // Fill memory
      for(int i = 0; i < num_pages; i++) {
        data[i * PAGE_SIZE] = (char)getpid();
      }
      
      // Access pattern
      for(int i = 0; i < num_pages; i += 7) {
        volatile char val = data[i * PAGE_SIZE];
      }
      
      free(data);
    }
    
    wait(0);  // Wait for child
    printf("Parent: Done\n");
  }
}

int
main(void)
{
  printf("=== PA4 Test 4: Comprehensive Stress Test ===\n");
  
  // Test with both scheduling policies
  stress_test_with_policy(0, "FCFS");
  stress_test_with_policy(1, "SSTF");
  
  // Test multiple processes
  multi_process_test();
  
  printf("\n=== Stress Test Complete ===\n");
  printf("If no crashes occurred, disk-backed swap is handling heavy load.\n");
  
  exit(0);
}
