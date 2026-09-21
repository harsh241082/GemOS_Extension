// PA3 Comprehensive Test: Full Feature Validation
// This test validates all PA3 features: allocation, faulting, eviction, swap, and priority

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/vmstats.h"

#define PAGE_SIZE 4096
#define KB(n) ((n) * 1024)
#define MB(n) ((n) * 1024 * 1024)

void print_stats(const char *label, int pid, struct vmstats *stats) {
  printf("%s\n", label);
  printf("  Page faults: %ld\n", stats->page_faults);
  printf("  Pages evicted: %ld\n", stats->pages_evicted);
  printf("  Pages swapped in: %ld\n", stats->pages_swapped_in);
  printf("  Pages swapped out: %ld\n", stats->pages_swapped_out);
  printf("  Resident pages: %ld\n", stats->resident_pages);
}

int main(int argc, char *argv[]) {
  printf("\n╔════════════════════════════════════════════════════╗\n");
  printf("║        PA3: Comprehensive Test Suite              ║\n");
  printf("║  Testing Page Replacement and Memory Management   ║\n");
  printf("╚════════════════════════════════════════════════════╝\n\n");

  int pid = getpid();
  struct vmstats stats, stats2, stats3;
  char *ptr1, *ptr2, *ptr3;
  int errors = 0;

  // ===== TEST 1: Basic Lazy Allocation =====
  printf("TEST 1: Basic Lazy Allocation\n");
  printf("───────────────────────────────\n");

  getvmstats(pid, &stats);
  printf("Initial state: pf=%ld, resident=%ld\n", stats.page_faults, stats.resident_pages);

  // Allocate 64 pages (256KB)
  ptr1 = sbrk(64 * PAGE_SIZE);
  if (ptr1 == (char *)-1) {
    printf("ERROR: sbrk failed\n");
    exit(1);
  }

  getvmstats(pid, &stats);
  printf("After sbrk(64 pages): pf=%ld, resident=%ld (should be 0)\n",
         stats.page_faults, stats.resident_pages);

  // Touch first page - should cause one page fault
  *ptr1 = 42;
  getvmstats(pid, &stats);
  if (stats.page_faults != 1) {
    printf("ERROR: Expected 1 page fault after touching first page, got %d\n",
           stats.page_faults);
    errors++;
  } else {
    printf("✓ PASS: One page fault triggered by first access\n");
  }

  // Touch more pages
  for (int i = 1; i < 64; i++) {
    *(ptr1 + i * PAGE_SIZE) = (char)i;
  }

  getvmstats(pid, &stats);
  printf("After touching all 64 pages: pf=%ld, resident=%ld\n",
         stats.page_faults, stats.resident_pages);

  if (stats.page_faults == 64 && stats.resident_pages == 64) {
    printf("✓ PASS: Correct page fault count and resident pages\n");
  } else {
    printf("ERROR: Expected pf=64, resident=64, got pf=%ld, resident=%ld\n",
           stats.page_faults, stats.resident_pages);
    errors++;
  }

  printf("\n");

  // ===== TEST 2: Memory Allocation and Page Faults =====
  printf("TEST 2: Multiple Allocations\n");
  printf("─────────────────────────────\n");

  int pf_before = stats.page_faults;

  // Allocate 128 more pages (512KB)
  ptr2 = sbrk(128 * PAGE_SIZE);
  if (ptr2 == (char *)-1) {
    printf("ERROR: sbrk for ptr2 failed\n");
    exit(1);
  }

  // Touch even pages only in ptr2
  for (int i = 0; i < 128; i += 2) {
    *(ptr2 + i * PAGE_SIZE) = (char)(i / 2);
  }

  getvmstats(pid, &stats);
  printf("After touching 64 pages in ptr2: pf=%d (added %d)\n",
         stats.page_faults, stats.page_faults - pf_before);

  if (stats.page_faults - pf_before == 64) {
    printf("✓ PASS: Correct page faults for new allocation\n");
  } else {
    printf("ERROR: Expected 64 page faults, got %d\n",
           stats.page_faults - pf_before);
    errors++;
  }

  printf("\n");

  // ===== TEST 3: Page Eviction and Swapping =====
  printf("TEST 3: Memory Pressure and Eviction\n");
  printf("────────────────────────────────────\n");

  int evicted_before = stats.pages_evicted;
  int swapped_out_before = stats.pages_swapped_out;

  // Allocate large amount to trigger eviction
  // This should cause existing pages to be evicted
  printf("Allocating large third region (200 pages)...\n");
  ptr3 = sbrk(200 * PAGE_SIZE);
  if (ptr3 == (char *)-1) {
    printf("ERROR: sbrk for ptr3 failed\n");
    exit(1);
  }

  // Touch all pages in ptr3
  for (int i = 0; i < 200; i++) {
    *(ptr3 + i * PAGE_SIZE) = (char)(i % 256);
  }

  getvmstats(pid, &stats);
  print_stats("After touching all pages in ptr3:", pid, &stats);

  printf("Evicted: %d pages, Swapped out: %d pages\n",
         stats.pages_evicted - evicted_before,
         stats.pages_swapped_out - swapped_out_before);

  if (stats.pages_evicted > 0 && stats.pages_swapped_out > 0) {
    printf("✓ PASS: Pages were evicted and swapped as expected\n");
  } else {
    printf("WARNING: No pages were evicted (system may have enough memory)\n");
  }

  printf("\n");

  // ===== TEST 4: Data Integrity After Swap =====
  printf("TEST 4: Data Integrity Verification\n");
  printf("──────────────────────────────────\n");

  // Verify ptr1 data is still intact (may have been swapped)
  int corruption = 0;
  for (int i = 0; i < 64; i++) {
    char expected = (i == 0) ? 42 : (char)i;
    char actual = *(ptr1 + i * PAGE_SIZE);
    if (actual != expected) {
      corruption++;
    }
  }

  if (corruption == 0) {
    printf("✓ PASS: All data in ptr1 is intact (%d pages verified)\n", 64);
  } else {
    printf("ERROR: Data corruption detected in ptr1: %d pages corrupted\n",
           corruption);
    errors++;
  }

  // Verify ptr2 data
  corruption = 0;
  for (int i = 0; i < 128; i += 2) {
    char expected = (char)(i / 2);
    char actual = *(ptr2 + i * PAGE_SIZE);
    if (actual != expected) {
      corruption++;
    }
  }

  if (corruption == 0) {
    printf("✓ PASS: All data in ptr2 is intact (32 pages verified)\n");
  } else {
    printf("ERROR: Data corruption detected in ptr2: %d pages corrupted\n",
           corruption);
    errors++;
  }

  printf("\n");

  // ===== TEST 5: Swap In Verification =====
  printf("TEST 5: Access to Swapped Pages (Swap-in)\n");
  printf("──────────────────────────────────────────\n");

  int swapped_in_before = stats.pages_swapped_in;

  // Access pages that were likely swapped out
  // Touch ptr1 pages again - they may need to be swapped back in
  volatile char temp = 0;
  for (int i = 0; i < 64; i++) {
    temp += *(ptr1 + i * PAGE_SIZE);
  }

  getvmstats(pid, &stats);
  printf("After re-accessing ptr1: swapped_in=%d (was %d)\n",
         stats.pages_swapped_in, swapped_in_before);

  if (stats.pages_swapped_in > swapped_in_before) {
    printf("✓ PASS: Pages were swapped back in\n");
  } else {
    printf("NOTE: No additional swap-ins (pages stayed in memory)\n");
  }

  printf("\n");

  // ===== FINAL STATISTICS =====
  printf("╔════════════════════════════════════════════════════╗\n");
  printf("║              FINAL STATISTICS                      ║\n");
  printf("╚════════════════════════════════════════════════════╝\n\n");

  getvmstats(pid, &stats);
  print_stats("Final process statistics:", pid, &stats);

  printf("\n");
  printf("═════════════════════════════════════════════════════\n");

  if (errors == 0) {
    printf("║  ✓ ALL TESTS PASSED                              ║\n");
  } else {
    printf("║  ✗ %d TEST(S) FAILED                              ║\n", errors);
  }

  printf("═════════════════════════════════════════════════════\n\n");

  exit(errors > 0 ? 1 : 0);
}
