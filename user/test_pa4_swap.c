// PA4 Test 1: Basic Disk-Backed Swap Test
// Allocates large memory region to trigger swap operations
// and verifies that swapped pages can be recovered

#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "user/test_pa4.h"

#define PAGE_SIZE 4096
#define NUM_PAGES 256  // 1MB allocation
#define TEST_PATTERN 0xDEADBEEF

int
main(void)
{
  printf("=== PA4 Test 1: Basic Disk-Backed Swap ===\n\n");
  
  struct vmstats stats_before, stats_after;
  int pid = getpid();
  
  // Get initial stats
  getvmstats(pid, &stats_before);
  printf("Before allocation:\n");
  printf("  Pages swapped out: %d\n", stats_before.pages_swapped_out);
  printf("  Pages swapped in: %d\n", stats_before.pages_swapped_in);
  printf("  Pages evicted: %d\n", stats_before.pages_evicted);
  printf("  Resident pages: %d\n", stats_before.resident_pages);
  printf("\n");
  
  // Allocate large memory region
  printf("Allocating %d MB...\n", (NUM_PAGES * PAGE_SIZE) / (1024 * 1024));
  char *data = (char *)malloc(NUM_PAGES * PAGE_SIZE);
  
  if(data == 0) {
    printf("FAIL: malloc failed\n");
    exit(1);
  }
  
  // Write pattern to all pages to trigger allocation
  printf("Writing test pattern to all pages...\n");
  for(int i = 0; i < NUM_PAGES; i++) {
    int offset = i * PAGE_SIZE;
    for(int j = 0; j < PAGE_SIZE; j += 4) {
      *(int *)(data + offset + j) = TEST_PATTERN + i;
    }
  }
  
  // Get stats after allocation
  getvmstats(pid, &stats_after);
  printf("After allocation:\n");
  printf("  Pages swapped out: %d\n", stats_after.pages_swapped_out);
  printf("  Pages swapped in: %d\n", stats_after.pages_swapped_in);
  printf("  Pages evicted: %d\n", stats_after.pages_evicted);
  printf("  Resident pages: %d\n", stats_after.resident_pages);
  printf("\n");
  
  // Verify data integrity by reading back
  printf("Verifying data integrity...\n");
  int verified = 0;
  int corrupted = 0;
  
  for(int i = 0; i < NUM_PAGES; i++) {
    int offset = i * PAGE_SIZE;
    int val = *(int *)(data + offset);
    if(val == TEST_PATTERN + i) {
      verified++;
    } else {
      corrupted++;
      if(corrupted <= 5) {  // Print first 5 errors
        printf("  Page %d corrupted: expected %x, got %x\n", 
               i, TEST_PATTERN + i, val);
      }
    }
  }
  
  printf("\nVerification results:\n");
  printf("  Verified pages: %d\n", verified);
  printf("  Corrupted pages: %d\n", corrupted);
  
  if(stats_after.pages_swapped_out > 0 && corrupted == 0) {
    printf("\n[PASS] Disk-backed swap is working!\n");
    printf("  Successfully swapped out %d pages\n", stats_after.pages_swapped_out);
    printf("  Successfully swapped in %d pages\n", stats_after.pages_swapped_in);
  } else {
    printf("\n[FAIL] Disk-backed swap not working properly\n");
  }
  
  free(data);
  exit(0);
}
