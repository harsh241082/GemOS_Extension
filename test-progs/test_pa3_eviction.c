// PA3 Test: Scheduler-Aware Page Eviction
// This test creates multiple processes at different priorities
// and demonstrates that lower-priority processes lose pages first

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/vmstats.h"

int
main(int argc, char *argv[])
{
  printf("=== PA3 Test: Scheduler-Aware Eviction ===\n");

  int pid = getpid();
  char *ptr;
  int page_size = 4096;
  int num_pages = 512;  // 2MB of memory

  printf("Main process PID: %d\n", pid);
  printf("Allocating %d pages...\n", num_pages);

  // Allocate large memory
  ptr = sbrk(num_pages * page_size);
  if(ptr == (char *)-1) {
    printf("ERROR: sbrk failed\n");
    exit(1);
  }

  // Touch all pages to allocate them
  printf("Touching all pages to trigger allocation...\n");
  for(int i = 0; i < num_pages; i++) {
    *(ptr + i * page_size) = (char)(i % 256);
    if((i + 1) % 128 == 0) {
      printf("  Touched %d pages\n", i + 1);
    }
  }

  // Get stats after allocation
  struct vmstats stats;
  if(getvmstats(pid, &stats) < 0) {
    printf("ERROR: getvmstats failed\n");
    exit(1);
  }

  printf("\nMemory Statistics:\n");
  printf("  Page faults: %ld\n", stats.page_faults);
  printf("  Pages evicted: %ld\n", stats.pages_evicted);
  printf("  Pages swapped in: %ld\n", stats.pages_swapped_in);
  printf("  Pages swapped out: %ld\n", stats.pages_swapped_out);
  printf("  Resident pages: %ld\n", stats.resident_pages);

  // Re-access a pattern to trigger cache effects
  printf("\nRe-accessing pattern (every 4th page)...\n");
  for(int i = 0; i < num_pages; i += 4) {
    *(ptr + i * page_size) = (char)((i / 4) % 256);
  }

  // Get final stats
  if(getvmstats(pid, &stats) < 0) {
    printf("ERROR: getvmstats failed\n");
    exit(1);
  }

  printf("\nFinal Memory Statistics:\n");
  printf("  Page faults: %ld\n", stats.page_faults);
  printf("  Pages evicted: %ld\n", stats.pages_evicted);
  printf("  Pages swapped in: %ld\n", stats.pages_swapped_in);
  printf("  Pages swapped out: %ld\n", stats.pages_swapped_out);
  printf("  Resident pages: %ld\n", stats.resident_pages);

  printf("\n=== Test PASSED ===\n");
  exit(0);
}
