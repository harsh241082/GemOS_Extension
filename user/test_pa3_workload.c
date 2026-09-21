// PA3 Test: Workload Stress Test
// This test creates a realistic workload with multiple allocations and accesses

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/vmstats.h"

int
main(int argc, char *argv[])
{
  printf("=== PA3 Test: Workload Stress Test ===\n");
  
  int pid = getpid();
  printf("PID: %d\n", pid);
  
  int page_size = 4096;
  char *allocations[5];
  int allocation_sizes[] = {64, 128, 96, 160, 100};  // Different sizes
  
  struct vmstats stats;
  
  // Phase 1: Sequential allocations
  printf("\n--- Phase 1: Sequential Allocations ---\n");
  for(int i = 0; i < 5; i++) {
    allocations[i] = sbrk(allocation_sizes[i] * page_size);
    if(allocations[i] == (char *)-1) {
      printf("ERROR: Allocation %d failed\n", i);
      exit(1);
    }
    printf("Allocation %d: %d pages at 0x%lx\n", i, allocation_sizes[i], (uint64)allocations[i]);
    
    // Touch all pages in this allocation
    for(int j = 0; j < allocation_sizes[i]; j++) {
      *(allocations[i] + j * page_size) = (char)((i * 100 + j) % 256);
    }
  }
  
  getvmstats(pid, &stats);
  printf("After allocations: pf=%ld, evicted=%ld, resident=%ld\\n",
         stats.page_faults, stats.pages_evicted, stats.resident_pages);
  
  // Phase 2: Random access pattern
  printf("\n--- Phase 2: Random Access Pattern ---\n");
  int access_count = 0;
  for(int iter = 0; iter < 3; iter++) {
    for(int i = 0; i < 5; i++) {
      // Access every other page
      for(int j = 0; j < allocation_sizes[i]; j += 2) {
        volatile char c = *(allocations[i] + j * page_size);
        access_count++;
      }
    }
  }
  printf("Performed %d accesses\n", access_count);
  
  getvmstats(pid, &stats);
  printf("After random access: pf=%ld, evicted=%ld, swapped_in=%ld, resident=%ld\\n",
         stats.page_faults, stats.pages_evicted, stats.pages_swapped_in, stats.resident_pages);
  
  // Phase 3: Working set pattern (access only a subset)
  printf("\n--- Phase 3: Working Set Access ---\n");
  int working_set_accesses = 0;
  
  // Access only allocations 0 and 2 repeatedly
  for(int iter = 0; iter < 10; iter++) {
    // Access allocation 0
    for(int j = 0; j < allocation_sizes[0]; j += 4) {
      *(allocations[0] + j * page_size) = (char)(iter % 256);
      working_set_accesses++;
    }
    
    // Access allocation 2
    for(int j = 0; j < allocation_sizes[2]; j += 4) {
      *(allocations[2] + j * page_size) = (char)((iter + 1) % 256);
      working_set_accesses++;
    }
  }
  printf("Performed %d working set accesses\n", working_set_accesses);
  
  getvmstats(pid, &stats);
  printf("After working set: pf=%ld, evicted=%ld, swapped_in=%ld, resident=%ld\\n",
         stats.page_faults, stats.pages_evicted, stats.pages_swapped_in, stats.resident_pages);
  
  // Phase 4: Data verification
  printf("\n--- Phase 4: Data Verification ---\n");
  int verification_errors = 0;
  
  for(int i = 0; i < 5; i++) {
    for(int j = 0; j < allocation_sizes[i]; j++) {
      char expected = (char)((i * 100 + j) % 256);
      char actual = *(allocations[i] + j * page_size);
      if(actual != expected) {
        verification_errors++;
        if(verification_errors <= 5) {  // Print first 5 errors
          printf("ERROR: Data mismatch at alloc %d, page %d: expected %d, got %d\n",
                 i, j, expected, actual);
        }
      }
    }
  }
  
  printf("Data verification: %d errors\n", verification_errors);
  
  getvmstats(pid, &stats);
  printf("\nFinal statistics:\n");
  printf("  Page faults: %ld\n", stats.page_faults);
  printf("  Pages evicted: %ld\n", stats.pages_evicted);
  printf("  Pages swapped in: %ld\n", stats.pages_swapped_in);
  printf("  Pages swapped out: %ld\n", stats.pages_swapped_out);
  printf("  Resident pages: %ld\n", stats.resident_pages);
  
  if(verification_errors == 0) {
    printf("\n=== Test PASSED ===\n");
    exit(0);
  } else {
    printf("\n=== Test FAILED ===\n");
    exit(1);
  }
}
