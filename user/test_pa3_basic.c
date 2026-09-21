// Basic test for PA3: Page Replacement
// This test allocates large memory regions and accesses them to trigger page faults

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/vmstats.h"

int
main(int argc, char *argv[])
{
  int pid = getpid();
  char *ptr;
  int page_size = 4096;
  int num_pages = 256;  // 1MB of memory
  struct vmstats before, after;

  printf("=== PA3 Basic Test: Memory Allocation and Paging ===\n");
  printf("PID: %d\n", pid);

  // Get initial stats
  if(getvmstats(pid, &before) < 0) {
    printf("ERROR: getvmstats failed\n");
    exit(1);
  }
  
  printf("Initial stats:\n");
  printf("  Page faults: %ld\n", before.page_faults);
  printf("  Pages evicted: %ld\n", before.pages_evicted);
  printf("  Pages swapped in: %ld\n", before.pages_swapped_in);
  printf("  Pages swapped out: %ld\n", before.pages_swapped_out);
  printf("  Resident pages: %ld\n", before.resident_pages);

  // Allocate large memory region
  printf("\nAllocating %d pages (%d bytes)...\n", num_pages, num_pages * page_size);
  ptr = sbrk(num_pages * page_size);
  if(ptr == (char *)-1) {
    printf("ERROR: sbrk failed\n");
    exit(1);
  }

  printf("Allocated at: 0x%lx\n", (uint64)ptr);

  // Access pages sequentially to trigger page faults
  printf("Accessing pages sequentially...\n");
  for(int i = 0; i < num_pages; i++) {
    *(ptr + i * page_size) = (char)(i % 256);
    if((i + 1) % 64 == 0) {
      printf("  Accessed %d pages\n", i + 1);
    }
  }

  // Get final stats
  if(getvmstats(pid, &after) < 0) {
    printf("ERROR: getvmstats failed\n");
    exit(1);
  }

  printf("\nFinal stats:\n");
  printf("  Page faults: %ld\n", after.page_faults);
  printf("  Pages evicted: %ld\n", after.pages_evicted);
  printf("  Pages swapped in: %ld\n", after.pages_swapped_in);
  printf("  Pages swapped out: %ld\n", after.pages_swapped_out);
  printf("  Resident pages: %ld\n", after.resident_pages);

  printf("\nDeltas:\n");
  printf("  Page faults: %ld\n", after.page_faults - before.page_faults);
  printf("  Pages evicted: %ld\n", after.pages_evicted - before.pages_evicted);
  printf("  Pages swapped in: %ld\n", after.pages_swapped_in - before.pages_swapped_in);
  printf("  Pages swapped out: %ld\n", after.pages_swapped_out - before.pages_swapped_out);
  printf("  Resident pages: %ld\n", after.resident_pages - before.resident_pages);

  printf("\n=== Test PASSED ===\n");
  exit(0);
}
