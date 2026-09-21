#ifndef VMSTATS_H
#define VMSTATS_H

struct vmstats {
  uint64 page_faults;      // Number of page faults
  uint64 pages_evicted;    // Pages evicted due to replacement
  uint64 pages_swapped_out; // Pages written to swap
  uint64 pages_swapped_in;  // Pages read from swap
  uint64 resident_pages;    // Currently resident pages
};

#endif
