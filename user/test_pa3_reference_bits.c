// PA3 Advanced Test: Clock Algorithm Visualization
// Tests the Clock algorithm behavior and eviction patterns

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/vmstats.h"

#define PAGE_SIZE 4096
#define NUM_ALLOCATIONS 5

int main(int argc, char *argv[]) {
  printf("\n╔═══════════════════════════════════════════════════╗\n");
  printf("║     PA3: Clock Algorithm Behavior Test           ║\n");
  printf("║   Observing eviction patterns and reference bits ║\n");
  printf("╚═══════════════════════════════════════════════════╝\n\n");

  int pid = getpid();
  struct vmstats stats;
  char *allocations[NUM_ALLOCATIONS];
  int allocation_sizes[] = {80, 80, 80, 80, 80};  // 5 allocations of 80 pages each
  
  printf("Test configuration:\n");
  printf("  Total allocations: %d\n", NUM_ALLOCATIONS);
  printf("  Pages per allocation: ");
  for(int i = 0; i < NUM_ALLOCATIONS; i++) {
    printf("%d ", allocation_sizes[i]);
  }
  printf("(total: %d pages)\n", allocation_sizes[0] * NUM_ALLOCATIONS);
  printf("  Each page: %d bytes\n", PAGE_SIZE);
  printf("\n");

  // ===== PHASE 1: Multiple Allocations =====
  printf("PHASE 1: Sequential Allocations\n");
  printf("────────────────────────────────\n");

  for(int i = 0; i < NUM_ALLOCATIONS; i++) {
    allocations[i] = sbrk(allocation_sizes[i] * PAGE_SIZE);
    if(allocations[i] == (char *)-1) {
      printf("ERROR: Allocation %d failed\n", i);
      exit(1);
    }
    printf("Allocation %d: %d pages at 0x%lx\n", i, allocation_sizes[i], (uint64)allocations[i]);
  }

  getvmstats(pid, &stats);
  printf("After all allocations: pf=%ld, resident=%ld (all lazy)\n\n",
         stats.page_faults, stats.resident_pages);

  // ===== PHASE 2: Sequential Access =====
  printf("PHASE 2: Sequential Access Pattern\n");
  printf("──────────────────────────────────\n");
  printf("Touching pages in sequence (simulates stream workload)...\n\n");

  for(int alloc = 0; alloc < NUM_ALLOCATIONS; alloc++) {
    printf("  Accessing allocation %d (pages %d-%d)...\n",
           alloc, alloc * allocation_sizes[alloc],
           (alloc + 1) * allocation_sizes[alloc] - 1);

    // Touch every page in this allocation
    for(int page = 0; page < allocation_sizes[alloc]; page++) {
      *(allocations[alloc] + page * PAGE_SIZE) = (char)(alloc * 100 + page);
    }

    getvmstats(pid, &stats);
    if((alloc + 1) % 1 == 0) {  // Print every allocation
      printf("    After: pf=%ld, evicted=%ld, resident=%ld, swapped_out=%ld\n",
             stats.page_faults, stats.pages_evicted,
             stats.resident_pages, stats.pages_swapped_out);
    }
  }

  getvmstats(pid, &stats);
  printf("\nSummary of Phase 2:\n");
  printf("  Total page faults: %ld\n", stats.page_faults);
  printf("  Total pages evicted: %ld\n", stats.pages_evicted);
  printf("  Pages swapped out: %ld\n", stats.pages_swapped_out);
  printf("  Resident pages: %ld\n\n", stats.resident_pages);

  // ===== PHASE 3: Repeated Access Pattern =====
  printf("PHASE 3: Repeated Access (Working Set Test)\n");
  printf("──────────────────────────────────────────\n");
  printf("Repeatedly accessing pages to test reference bits...\n\n");

  int initial_evicted = stats.pages_evicted;
  int initial_swapped_out = stats.pages_swapped_out;

  // Access pages multiple times - should reuse them
  for(int iter = 0; iter < 2; iter++) {
    printf("  Iteration %d:\n", iter + 1);

    for(int alloc = 0; alloc < NUM_ALLOCATIONS; alloc++) {
      // Touch first 20 pages of each allocation
      for(int page = 0; page < 20 && page < allocation_sizes[alloc]; page++) {
        volatile char c = *(allocations[alloc] + page * PAGE_SIZE);
      }
    }

    getvmstats(pid, &stats);
    printf("    After iteration: pf=%ld, resident=%ld, evicted=%ld (this iteration: %ld)\n",
           stats.page_faults, stats.resident_pages, stats.pages_evicted,
           stats.pages_evicted - initial_evicted);
  }

  printf("\n");

  // ===== PHASE 4: Data Integrity Check =====
  printf("PHASE 4: Data Integrity Verification\n");
  printf("──────────────────────────────────────\n");

  int total_corruption = 0;

  for(int alloc = 0; alloc < NUM_ALLOCATIONS; alloc++) {
    int corruption_count = 0;

    // Verify all pages in this allocation
    for(int page = 0; page < allocation_sizes[alloc]; page++) {
      char expected = (char)(alloc * 100 + page);
      char actual = *(allocations[alloc] + page * PAGE_SIZE);

      if(actual != expected) {
        corruption_count++;
      }
    }

    if(corruption_count == 0) {
      printf("  Allocation %d: ✓ PASS (%d pages verified)\n",
             alloc, allocation_sizes[alloc]);
    } else {
      printf("  Allocation %d: ✗ FAIL (%d/%d pages corrupted)\n",
             alloc, corruption_count, allocation_sizes[alloc]);
      total_corruption += corruption_count;
    }
  }

  printf("\n");

  // ===== PHASE 5: Sparse Access Pattern =====
  printf("PHASE 5: Sparse Access (Tests Reference Bit Clearing)\n");
  printf("───────────────────────────────────────────────────\n");

  int pf_before_sparse = stats.page_faults;

  printf("  Accessing every 4th page in all allocations...\n");

  for(int alloc = 0; alloc < NUM_ALLOCATIONS; alloc++) {
    for(int page = 0; page < allocation_sizes[alloc]; page += 4) {
      volatile char c = *(allocations[alloc] + page * PAGE_SIZE);
    }
  }

  printf("  Now accessing pages that weren't accessed before...\n");

  for(int alloc = 0; alloc < NUM_ALLOCATIONS; alloc++) {
    for(int page = 1; page < allocation_sizes[alloc]; page += 4) {
      volatile char c = *(allocations[alloc] + page * PAGE_SIZE);
    }
  }

  getvmstats(pid, &stats);
  printf("  Additional page faults: %ld\n", stats.page_faults - pf_before_sparse);
  printf("\n");

  // ===== FINAL REPORT =====
  printf("╔═══════════════════════════════════════════════════╗\n");
  printf("║              FINAL TEST REPORT                    ║\n");
  printf("╚═══════════════════════════════════════════════════╝\n\n");

  getvmstats(pid, &stats);

  printf("Final Statistics:\n");
  printf("  Page faults: %ld\n", stats.page_faults);
  printf("  Pages evicted: %ld\n", stats.pages_evicted);
  printf("  Pages swapped out: %ld\n", stats.pages_swapped_out);
  printf("  Pages swapped in: %ld\n", stats.pages_swapped_in);
  printf("  Resident pages: %ld\n\n", stats.resident_pages);

  printf("Behavior Analysis:\n");
  if(stats.pages_evicted > 0) {
    printf("  ✓ Clock algorithm triggered evictions\n");
    printf("    Eviction rate: %.1f%% of faults\n",
           (100.0 * stats.pages_evicted) / stats.page_faults);
  } else {
    printf("  ✓ No evictions needed (sufficient physical memory)\n");
  }

  if(stats.pages_swapped_in > 0) {
    printf("  ✓ Swapped pages were restored successfully\n");
  }

  if(total_corruption == 0) {
    printf("  ✓ All data integrity verified - NO CORRUPTION\n");
  } else {
    printf("  ✗ ERROR: Data corruption detected!\n");
  }

  printf("\n");
  printf("═══════════════════════════════════════════════════\n");
  printf("Test Result: %s\n", total_corruption == 0 ? "PASS ✓" : "FAIL ✗");
  printf("═══════════════════════════════════════════════════\n\n");

  exit(total_corruption > 0 ? 1 : 0);
}
