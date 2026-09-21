// PA3 Test: Clock Algorithm Behavior
// This test verifies the CLOCK page replacement algorithm behavior

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/vmstats.h"

int
main(int argc, char *argv[])
{
  printf("=== PA3 Test: CLOCK Algorithm Verification ===\n");
  
  int pid = getpid();
  printf("PID: %d\n\n", pid);
  
  int page_size = 4096;
  
  // Test 1: Sequential allocation and eviction
  printf("--- Test 1: Sequential Allocation Pattern ---\n");
  
  char *mem1 = sbrk(128 * page_size);  // 512KB
  if(mem1 == (char *)-1) {
    printf("ERROR: mem1 allocation failed\n");
    exit(1);
  }
  printf("Allocated mem1: 128 pages\n");
  
  // Fill with pattern
  for(int i = 0; i < 128; i++) {
    *(mem1 + i * page_size) = (char)(i % 256);
  }
  
  struct vmstats stats1;
  getvmstats(pid, &stats1);
  printf("After mem1: pf=%ld, resident=%ld, evicted=%ld\\n",
         stats1.page_faults, stats1.resident_pages, stats1.pages_evicted);
  
  // Test 2: Allocate more to trigger eviction
  printf("\n--- Test 2: Trigger Eviction ---\n");
  
  char *mem2 = sbrk(64 * page_size);  // 256KB
  if(mem2 == (char *)-1) {
    printf("ERROR: mem2 allocation failed\n");
    exit(1);
  }
  printf("Allocated mem2: 64 pages\n");
  
  for(int i = 0; i < 64; i++) {
    *(mem2 + i * page_size) = (char)(i % 256);
  }
  
  struct vmstats stats2;
  getvmstats(pid, &stats2);
  printf("After mem2: pf=%ld, resident=%ld, evicted=%ld\\n",
         stats2.page_faults, stats2.resident_pages, stats2.pages_evicted);
  
  // Test 3: Reference bit clearing behavior
  printf("\n--- Test 3: Reference Bit Behavior ---\n");
  printf("Allocating mem3 to further trigger eviction...\n");
  
  char *mem3 = sbrk(96 * page_size);  // 384KB
  if(mem3 == (char *)-1) {
    printf("ERROR: mem3 allocation failed\n");
    exit(1);
  }
  
  for(int i = 0; i < 96; i++) {
    *(mem3 + i * page_size) = (char)(i % 256);
  }
  
  struct vmstats stats3;
  getvmstats(pid, &stats3);
  printf("After mem3: pf=%ld, resident=%ld, evicted=%ld\\n",
         stats3.page_faults, stats3.resident_pages, stats3.pages_evicted);
  
  // Test 4: Re-access pattern to trigger swap-in
  printf("\n--- Test 4: Swap-in Behavior ---\n");
  printf("Re-accessing mem1 (which should be partially swapped)...\n");
  
  int accessed = 0;
  int faults_before = stats3.page_faults;
  
  for(int i = 0; i < 128; i += 8) {  // Access every 8th page
    *(mem1 + i * page_size) = (char)((i + 1) % 256);
    accessed++;
  }
  
  struct vmstats stats4;
  getvmstats(pid, &stats4);
  printf("After re-access: pf=%ld, swapped_in=%ld\\n",
         stats4.page_faults, stats4.pages_swapped_in);
  printf("Accessed %d pages, page faults: %ld\\n", accessed,
         stats4.page_faults - faults_before);
  
  // Test 5: Data integrity check
  printf("\n--- Test 5: Data Integrity Check ---\n");
  
  int errors = 0;
  
  // Check mem1
  for(int i = 0; i < 128; i++) {
    char expected = (i % 8 == 0) ? (char)((i + 1) % 256) : (char)(i % 256);
    char actual = *(mem1 + i * page_size);
    if(actual != expected && actual != (char)(i % 256)) {
      if(errors < 5) {
        printf("mem1[%d]: expected %d, got %d\n", i, expected, actual);
      }
      errors++;
    }
  }
  
  // Check mem2
  for(int i = 0; i < 64; i++) {
    char expected = (char)(i % 256);
    char actual = *(mem2 + i * page_size);
    if(actual != expected) {
      if(errors < 10) {
        printf("mem2[%d]: expected %d, got %d\n", i, expected, actual);
      }
      errors++;
    }
  }
  
  // Check mem3
  for(int i = 0; i < 96; i++) {
    char expected = (char)(i % 256);
    char actual = *(mem3 + i * page_size);
    if(actual != expected) {
      if(errors < 15) {
        printf("mem3[%d]: expected %d, got %d\n", i, expected, actual);
      }
      errors++;
    }
  }
  
  printf("Data integrity errors: %d\n", errors);
  
  // Final statistics
  printf("\n--- Final Statistics ---\n");
  struct vmstats final_stats;
  getvmstats(pid, &final_stats);
  printf("  Page faults: %ld\n", final_stats.page_faults);
  printf("  Pages evicted: %ld\n", final_stats.pages_evicted);
  printf("  Pages swapped in: %ld\n", final_stats.pages_swapped_in);
  printf("  Pages swapped out: %ld\n", final_stats.pages_swapped_out);
  printf("  Resident pages: %ld\n", final_stats.resident_pages);
  
  if(errors == 0) {
    printf("\n=== Test PASSED ===\n");
    exit(0);
  } else {
    printf("\n=== Test FAILED (Errors: %d) ===\n", errors);
    exit(1);
  }
}
