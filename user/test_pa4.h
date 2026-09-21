// PA4 test utilities and structures

#ifndef TEST_PA4_H
#define TEST_PA4_H

#include "kernel/vmstats.h"

// RAID and disk constants (from kernel)
#define DISK_SCHED_FCFS 0
#define DISK_SCHED_SSTF 1
#define NUM_DISKS 4

// Test result structure
struct test_result {
  char *name;
  int passed;
  int failed;
  const char *details;
};

// Helper function to print test results
void print_test_result(struct test_result *result) {
  if(result->passed) {
    printf("[PASS] %s\n", result->name);
  } else {
    printf("[FAIL] %s: %s\n", result->name, result->details);
  }
}

// Get current disk statistics
void get_disk_info(int pid, struct vmstats *stats) {
  getvmstats(pid, stats);
}

#endif // TEST_PA4_H
