// PA4 Test 2: Disk Scheduling Policies (FCFS vs SSTF)
// Tests setdisksched() and compares performance

#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "user/test_pa4.h"

#define MB(x) ((x) * 1024 * 1024)
#define PAGE_SIZE 4096

typedef struct {
  int policy;
  const char *name;
  int pages_swapped;
  int uptime_start;
  int uptime_end;
} sched_test_t;

void run_memory_intensive_workload(int num_pages) {
  char *data = (char *)malloc(num_pages * PAGE_SIZE);
  if(!data) {
    printf("FAIL: malloc failed\n");
    return;
  }
  
  // Sequential access pattern
  printf("  Running sequential memory pattern...\n");
  for(int i = 0; i < num_pages; i++) {
    int offset = i * PAGE_SIZE;
    for(int j = 0; j < PAGE_SIZE; j++) {
      data[offset + j] = (char)(i + j);
    }
  }
  
  // Random access pattern
  printf("  Running random memory pattern...\n");
  for(int iter = 0; iter < 10; iter++) {
    for(int i = 0; i < num_pages; i += 7) {
      int offset = (i * PAGE_SIZE) % (num_pages * PAGE_SIZE);
      data[offset] = (char)(iter * i);
    }
  }
  
  free(data);
}

int
main(void)
{
  printf("=== PA4 Test 2: Disk Scheduling Policies ===\n\n");
  
  sched_test_t tests[2] = {
    {DISK_SCHED_FCFS, "FCFS", 0, 0, 0},
    {DISK_SCHED_SSTF, "SSTF", 0, 0, 0}
  };
  
  int pid = getpid();
  
  // Test each scheduling policy
  for(int t = 0; t < 2; t++) {
    printf("Testing %s scheduling:\n", tests[t].name);
    
    // Set scheduling policy
    if(setdisksched(tests[t].policy) < 0) {
      printf("  FAIL: setdisksched(%d) failed\n", tests[t].policy);
      continue;
    }
    printf("  Set policy to %s\n", tests[t].name);
    
    // Get initial statistics
    struct vmstats stats_before;
    getvmstats(pid, &stats_before);
    tests[t].uptime_start = uptime();
    
    // Run workload
    printf("  Running memory-intensive workload...\n");
    run_memory_intensive_workload(512);  // 2MB of pages
    
    // Get final statistics
    tests[t].uptime_end = uptime();
    struct vmstats stats_after;
    getvmstats(pid, &stats_after);
    
    tests[t].pages_swapped = stats_after.pages_swapped_out - stats_before.pages_swapped_out;
    
    printf("  Results:\n");
    printf("    Pages swapped out: %d\n", tests[t].pages_swapped);
    printf("    Pages swapped in: %d\n", 
           stats_after.pages_swapped_in - stats_before.pages_swapped_in);
    printf("    Ticks elapsed: %d\n", tests[t].uptime_end - tests[t].uptime_start);
    printf("\n");
  }
  
  // Compare results
  printf("Comparison:\n");
  if(tests[0].pages_swapped > 0 && tests[1].pages_swapped > 0) {
    int fcfs_time = tests[0].uptime_end - tests[0].uptime_start;
    int sstf_time = tests[1].uptime_end - tests[1].uptime_start;
    printf("  FCFS time: %d ticks\n", fcfs_time);
    printf("  SSTF time: %d ticks\n", sstf_time);
    
    if(sstf_time < fcfs_time) {
      printf("  [GOOD] SSTF is faster (%.1f%% improvement)\n", 
             100.0 * (fcfs_time - sstf_time) / fcfs_time);
    } else if(fcfs_time < sstf_time) {
      printf("  [INFO] FCFS is faster (might be due to disk layout)\n");
    } else {
      printf("  [INFO] Both policies have similar performance\n");
    }
  }
  
  printf("\n[INFO] Disk scheduling test completed\n");
  
  exit(0);
}
