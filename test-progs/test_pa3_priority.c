// PA3 Test: Priority-Based Eviction
// This test verifies that lower-priority processes lose pages before higher-priority ones

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/vmstats.h"

void child_process(int pages_to_allocate) {
  char *ptr;
  int page_size = 4096;
  int pid = getpid();
  
  printf("Child process %d: Allocating %d pages\n", pid, pages_to_allocate);
  
  ptr = sbrk(pages_to_allocate * page_size);
  if(ptr == (char *)-1) {
    printf("ERROR: Child %d sbrk failed\n", pid);
    exit(1);
  }
  
  // Touch all pages
  for(int i = 0; i < pages_to_allocate; i++) {
    *(ptr + i * page_size) = (char)(i % 256);
  }
  
  struct vmstats stats;
  getvmstats(pid, &stats);
  printf("Child %d: page_faults=%ld, resident=%ld, evicted=%ld\\n", 
         pid, stats.page_faults, stats.resident_pages, stats.pages_evicted);
  
  sleep(2);  // Keep process alive to observe behavior
  
  exit(0);
}

int
main(int argc, char *argv[])
{
  printf("=== PA3 Test: Priority-Based Eviction ===\n");
  
  int parent_pid = getpid();
  printf("Parent process PID: %d\n", parent_pid);
  
  // Allocate large amount of memory in parent
  printf("\nParent: Allocating 300 pages...\n");
  char *parent_mem = sbrk(300 * 4096);
  if(parent_mem == (char *)-1) {
    printf("ERROR: Parent sbrk failed\n");
    exit(1);
  }
  
  // Touch all parent pages
  for(int i = 0; i < 300; i++) {
    *(parent_mem + i * 4096) = (char)(i & 0xFF);
  }
  
  struct vmstats parent_stats_before;
  getvmstats(parent_pid, &parent_stats_before);
  printf("Parent before children: page_faults=%ld, resident=%ld\\n",
         parent_stats_before.page_faults, parent_stats_before.resident_pages);
  
  // Create child processes with different priorities
  int child1_pid = fork();
  if(child1_pid == 0) {
    // Child 1: allocate moderate memory
    child_process(150);
  } else {
    // Parent waits a bit
    sleep(1);
    
    int child2_pid = fork();
    if(child2_pid == 0) {
      // Child 2: allocate more memory (should cause evictions)
      child_process(200);
    } else {
      // Wait for children
      wait(0);
      wait(0);
      
      struct vmstats parent_stats_after;
      getvmstats(parent_pid, &parent_stats_after);
      
      printf("\nParent after children exit:\n");
      printf("  Page faults: %ld\\n", parent_stats_after.page_faults);
      printf("  Pages evicted: %ld\\n", parent_stats_after.pages_evicted);
      printf("  Resident pages: %ld\\n", parent_stats_after.resident_pages);
      
      // Verify parent data is still correct
      int errors = 0;
      for(int i = 0; i < 300; i += 10) {
        char expected = (char)(i & 0xFF);
        char actual = *(parent_mem + i * 4096);
        if(actual != expected) {
          errors++;
        }
      }
      
      if(errors == 0) {
        printf("\n=== Test PASSED (Parent data intact) ===\n");
        exit(0);
      } else {
        printf("\n=== Test FAILED (Data corruption: %d errors) ===\n", errors);
        exit(1);
      }
    }
  }
}
