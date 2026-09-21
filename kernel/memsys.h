// Memory management system for page replacement and swapping

#ifndef MEMSYS_H
#define MEMSYS_H

#include "types.h"
#include "proc.h"

#define MAX_FRAMES 32768  // Maximum number of frames (128MB / 4KB)
#define MAX_SWAPPED_PAGES 32768  // Maximum swapped pages
#define MAX_SWAP_DISK_ENTRIES 32768  // Maximum disk-backed swap entries (PA4)
#define SWAP_PAGE_SIZE 4096  // Size of each swap slot

// Frame table entry
struct frame_entry {
  int in_use;           // 1 if frame is allocated, 0 if free
  struct proc *owner;   // Process that owns this page
  uint64 vaddr;         // Virtual address of the page in the process
  int ref_bit;          // Reference bit for Clock algorithm
  int swapped;          // 1 if page is in swap, 0 if in physical memory
  int swap_index;       // Index in swap space if swapped
};

// Disk-backed swap entry (PA4 - replaces in-memory swap_entry)
struct swap_disk_entry {
  int in_use;           // 1 if swap slot is used, 0 if free
  struct proc *owner;   // Process that owns this page
  uint64 vaddr;         // Virtual address of the page
  int logical_block;    // Logical block number on disk
  uint64 timestamp;     // When swapped out
};

// Global frame table
extern struct frame_entry frame_table[MAX_FRAMES];
extern int frame_table_size;
extern int clock_hand;  // Hand for Clock algorithm

// Global disk-backed swap space
extern struct swap_disk_entry swap_disk_table[MAX_SWAP_DISK_ENTRIES];
extern int swap_disk_size;
extern int next_swap_block;  // Next available disk logical block

// Function declarations
void frametable_init(void);
void swap_init(void);
void set_reference_bit(void *pa);
void clear_reference_bit(void *pa);
struct frame_entry* get_frame_entry(void *pa);
void *evict_page(void);
int swap_page_out(void *pa);
int swap_page_in(struct proc *p, uint64 va);
void update_frame_table(struct proc *p, uint64 va, void *pa);
void remove_from_frame_table(void *pa);

#endif
