// PA3 Test: Swap In/Out Verification
// Tests that pages are correctly swapped out and restored from swap

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/vmstats.h"

int
main(int argc, char *argv[])
{
  printf("=== PA3 Test: Swap In/Out Verification ===\n");

  int pid = getpid();
  char *mem1, *mem2;
  int page_size = 4096;
  int num_pages1 = 256;  // 1MB for first allocation
  int num_pages2 = 256;  // 1MB for second allocation
  struct vmstats stats;

  printf("PID: %d\n", pid);

  // First allocation and initialization
  printf("\n--- Phase 1: Initial Allocation ---\n");
  mem1 = sbrk(num_pages1 * page_size);
  if(mem1 == (char *)-1) {
    printf("ERROR: sbrk failed for mem1\n");
    exit(1);
  }

  printf("mem1 allocated at 0x%lx\n", (uint64)mem1);
  printf("Initializing mem1 with pattern...\n");
  
  for(int i = 0; i < num_pages1; i++) {
    *(mem1 + i * page_size) = (char)(i & 0xFF);
  }

  if(getvmstats(pid, &stats) < 0) {
    printf("ERROR: getvmstats failed\n");
    exit(1);
  }

  printf("Stats after mem1 allocation:\n");
  printf("  Page faults: %ld\n", stats.page_faults);
  printf("  Resident pages: %ld\n", stats.resident_pages);
  printf("  Pages swapped out: %ld\n", stats.pages_swapped_out);

  // Second allocation to trigger eviction
  printf("\n--- Phase 2: Second Allocation (Triggers Eviction) ---\n");
  mem2 = sbrk(num_pages2 * page_size);
  if(mem2 == (char *)-1) {
    printf("ERROR: sbrk failed for mem2\n");
    exit(1);
  }

  printf("mem2 allocated at 0x%lx\n", (uint64)mem2);
  printf("Initializing mem2 with pattern...\n");
  
  for(int i = 0; i < num_pages2; i++) {
    *(mem2 + i * page_size) = (char)(i >> 8);
  }

  if(getvmstats(pid, &stats) < 0) {
    printf("ERROR: getvmstats failed\n");
    exit(1);
  }

  printf("Stats after mem2 allocation:\n");
  printf("  Page faults: %ld\n", stats.page_faults);
  printf("  Pages evicted: %ld\n", stats.pages_evicted);
  printf("  Pages swapped out: %ld\n", stats.pages_swapped_out);
  printf("  Resident pages: %ld\n", stats.resident_pages);

  // Re-access mem1 to trigger swap-in
  printf("\n--- Phase 3: Re-access mem1 (Triggers Swap-In) ---\n");
  printf("Re-accessing mem1 pages...\n");
  
  int fault_count = 0;
  for(int i = 0; i < num_pages1; i += 4) {  // Access every 4th page
    char expected = (char)(i & 0xFF);
    char actual = *(mem1 + i * page_size);
    if(actual != expected) {
      printf("ERROR: Data mismatch at offset %d: expected %d, got %d\n", 
             i * page_size, expected, actual);
      fault_count++;
    }
  }

  if(getvmstats(pid, &stats) < 0) {
    printf("ERROR: getvmstats failed\n");
    exit(1);
  }

  printf("Stats after mem1 re-access:\n");
  printf("  Pages swapped in: %ld\n", stats.pages_swapped_in);
  printf("  Resident pages: %ld\n", stats.resident_pages);

  if(fault_count == 0) {
    printf("\n=== Test PASSED (All data consistent) ===\n");
  } else {
    printf("\n=== Test FAILED (Data mismatches: %d) ===\n", fault_count);
    exit(1);
  }

  exit(0);
}
