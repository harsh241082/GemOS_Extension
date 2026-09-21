// Memory management system for page replacement and disk-backed swapping (PA4)

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "proc.h"
#include "defs.h"
#include "memsys.h"
#include "vm.h"
#include "disk_sched.h"

// Extern from kernel.ld - used for kernel memory bounds
extern char end[];

// Global frame table
struct frame_entry frame_table[MAX_FRAMES];
int frame_table_size = 0;
int clock_hand = 0;
struct spinlock frame_lock;

// Disk-backed swap mapping: map from virtual address to disk block
// We use a simple structure to track swapped pages
struct swap_disk_entry {
  int in_use;           // 1 if swap slot is used
  struct proc *owner;   // Process that owns this page  
  uint64 vaddr;         // Virtual address of the page
  int logical_block;    // Logical block number on disk
  uint64 timestamp;     // When swapped out
};

#define MAX_SWAP_DISK_ENTRIES 32768
struct swap_disk_entry swap_disk_table[MAX_SWAP_DISK_ENTRIES];
int swap_disk_size = 0;
struct spinlock swap_disk_lock;

// Next available logical block for swap
int next_swap_block = SWAP_START_BLOCK;

// Initialize frame table
void
frametable_init(void)
{
  initlock(&frame_lock, "frame_lock");
  initlock(&swap_disk_lock, "swap_disk_lock");
  
  // Initialize frame table entries
  for(int i = 0; i < MAX_FRAMES; i++) {
    frame_table[i].in_use = 0;
    frame_table[i].owner = 0;
    frame_table[i].vaddr = 0;
    frame_table[i].ref_bit = 0;
    frame_table[i].swapped = 0;
    frame_table[i].swap_index = -1;
  }
  
  // Initialize swap disk table
  for(int i = 0; i < MAX_SWAP_DISK_ENTRIES; i++) {
    swap_disk_table[i].in_use = 0;
    swap_disk_table[i].owner = 0;
    swap_disk_table[i].vaddr = 0;
    swap_disk_table[i].logical_block = -1;
  }
  
  disk_sched_init();
}

// Initialize swap space
void
swap_init(void)
{
  // Already initialized in frametable_init
}

// Get frame table entry for a physical address
struct frame_entry*
get_frame_entry(void *pa)
{
  uint64 phys_addr = (uint64)pa;
  
  // Calculate frame index
  // Frames start after kernel memory ends
  uint64 kernel_end = (uint64)end;
  uint64 kernel_end_rounded = PGROUNDUP(kernel_end);
  
  if(phys_addr < kernel_end_rounded || phys_addr >= PHYSTOP)
    return 0;
  
  int frame_index = (phys_addr - kernel_end_rounded) / PGSIZE;
  
  if(frame_index < 0 || frame_index >= MAX_FRAMES)
    return 0;
  
  return &frame_table[frame_index];
}

// Convert frame index to physical address
void*
frame_index_to_pa(int index)
{
  uint64 kernel_end = (uint64)end;
  uint64 kernel_end_rounded = PGROUNDUP(kernel_end);
  return (void*)(kernel_end_rounded + (uint64)index * PGSIZE);
}

// Convert physical address to frame index
int
pa_to_frame_index(void *pa)
{
  uint64 phys_addr = (uint64)pa;
  uint64 kernel_end = (uint64)end;
  uint64 kernel_end_rounded = PGROUNDUP(kernel_end);
  
  if(phys_addr < kernel_end_rounded || phys_addr >= PHYSTOP)
    return -1;
  
  return (phys_addr - kernel_end_rounded) / PGSIZE;
}

// Set reference bit
void
set_reference_bit(void *pa)
{
  struct frame_entry *entry = get_frame_entry(pa);
  if(entry) {
    acquire(&frame_lock);
    entry->ref_bit = 1;
    release(&frame_lock);
  }
}

// Clear reference bit
void
clear_reference_bit(void *pa)
{
  struct frame_entry *entry = get_frame_entry(pa);
  if(entry) {
    acquire(&frame_lock);
    entry->ref_bit = 0;
    release(&frame_lock);
  }
}

// Update frame table entry when a page is allocated
void
update_frame_table(struct proc *p, uint64 va, void *pa)
{
  struct frame_entry *entry = get_frame_entry(pa);
  if(entry) {
    acquire(&frame_lock);
    entry->in_use = 1;
    entry->owner = p;
    entry->vaddr = va;
    entry->ref_bit = 1;  // New pages have ref bit set
    entry->swapped = 0;
    entry->swap_index = -1;
    release(&frame_lock);
  }
}

// Remove frame from frame table
void
remove_from_frame_table(void *pa)
{
  struct frame_entry *entry = get_frame_entry(pa);
  if(entry) {
    acquire(&frame_lock);
    entry->in_use = 0;
    entry->owner = 0;
    entry->vaddr = 0;
    entry->ref_bit = 0;
    entry->swapped = 0;
    entry->swap_index = -1;
    release(&frame_lock);
  }
}

// Clock page replacement algorithm
void*
evict_page(void)
{
  struct frame_entry *victim = 0;
  int lowest_priority = -1;
  int start_index = clock_hand;
  
  acquire(&frame_lock);
  
  while(1) {
    struct frame_entry *entry = &frame_table[clock_hand];
    
    // Skip kernel frames and unused frames
    if(entry->in_use && entry->owner != 0) {
      int priority = entry->owner->level;  // Lower level = higher priority
      
      if(entry->ref_bit == 0) {
        // Found a candidate with ref_bit = 0
        if(lowest_priority == -1 || priority > lowest_priority) {
          victim = entry;
          lowest_priority = priority;
          // If this is the lowest priority, we can use it immediately
          if(lowest_priority == MLFQ_LEVELS - 1) {
            break;
          }
        }
      } else {
        // Clear ref bit and continue
        entry->ref_bit = 0;
      }
    }
    
    clock_hand = (clock_hand + 1) % MAX_FRAMES;
    
    // Prevent infinite loop
    if(clock_hand == start_index && victim != 0) {
      break;
    }
  }
  
  if(victim == 0) {
    // If no victim found with ref_bit=0, scan again without clearing bits
    clock_hand = 0;
    for(int i = 0; i < MAX_FRAMES; i++) {
      struct frame_entry *entry = &frame_table[i];
      if(entry->in_use && entry->owner != 0) {
        int priority = entry->owner->level;
        if(lowest_priority == -1 || priority > lowest_priority) {
          victim = entry;
          lowest_priority = priority;
        }
      }
    }
  }
  
  release(&frame_lock);
  
  if(victim == 0)
    return 0;
  
  // Convert frame entry to physical address
  int index = victim - frame_table;
  void *pa = frame_index_to_pa(index);
  
  // Swap out the page
  if(swap_page_out(pa) != 0) {
    return 0;  // Failed to swap
  }
  
  // Update victim owner's statistics
  if(victim->owner) {
    victim->owner->pages_evicted++;
    victim->owner->pages_swapped_out++;
    victim->owner->resident_pages--;
  }
  
  return pa;
}

// Swap page out to disk
int
swap_page_out(void *pa)
{
  struct frame_entry *entry = get_frame_entry(pa);
  if(entry == 0 || entry->owner == 0)
    return -1;
  
  // Find free swap disk slot
  acquire(&swap_disk_lock);
  int swap_index = -1;
  for(int i = 0; i < MAX_SWAP_DISK_ENTRIES; i++) {
    if(swap_disk_table[i].in_use == 0) {
      swap_index = i;
      break;
    }
  }
  release(&swap_disk_lock);
  
  if(swap_index == -1)
    return -1;  // No free swap space
  
  // Get logical block number with bounds checking
  acquire(&swap_disk_lock);
  int logical_block = next_swap_block;
  
  // Check if we have reached maximum capacity
  if(logical_block >= NUM_DISKS * DISK_BLOCKS_PER_DISK) {
    release(&swap_disk_lock);
    return -1;  // Swap space exhausted
  }
  next_swap_block++;
  release(&swap_disk_lock);
  
  // Create a disk request to write this page
  // Buffer is the physical memory containing page data
  if(add_disk_request(DISK_WRITE, logical_block, pa, entry->owner) != 0) {
    return -1;
  }
  
  // Process the disk request
  struct disk_request *req = schedule_next_request();
  if(req) {
    process_disk_request(req);
    kfree((void *)req);
  }
  
  // Update swap disk table
  acquire(&swap_disk_lock);
  struct swap_disk_entry *swap_entry = &swap_disk_table[swap_index];
  swap_entry->in_use = 1;
  swap_entry->owner = entry->owner;
  swap_entry->vaddr = entry->vaddr;
  swap_entry->logical_block = logical_block;
  swap_entry->timestamp = ticks;
  release(&swap_disk_lock);
  
  // Update frame entry
  acquire(&frame_lock);
  entry->swapped = 1;
  entry->swap_index = swap_index;
  
  // Mark page as invalid in page table
  pte_t *pte = walk(entry->owner->pagetable, entry->vaddr, 0);
  if(pte) {
    *pte &= ~PTE_V;  // Clear valid bit
    // Store swap index in upper bits of PTE for later retrieval
    *pte |= ((uint64)swap_index << 10);
  }
  release(&frame_lock);
  
  return 0;
}

// Swap page in from disk
int
swap_page_in(struct proc *p, uint64 va)
{
  // Find the swap entry for this process and virtual address
  acquire(&swap_disk_lock);
  int swap_index = -1;
  int logical_block = -1;
  
  for(int i = 0; i < MAX_SWAP_DISK_ENTRIES; i++) {
    if(swap_disk_table[i].in_use && 
       swap_disk_table[i].owner == p && 
       swap_disk_table[i].vaddr == va) {
      swap_index = i;
      logical_block = swap_disk_table[i].logical_block;
      break;
    }
  }
  release(&swap_disk_lock);
  
  if(swap_index == -1)
    return -1;  // Page not in swap
  
  // Allocate new physical page
  void *new_pa = (void *)kalloc();
  if(new_pa == 0) {
    // Need to evict another page
    new_pa = evict_page();
    if(new_pa == 0)
      return -1;
  }
  
  // Create disk request to read page from disk
  if(add_disk_request(DISK_READ, logical_block, new_pa, p) != 0) {
    kfree(new_pa);
    return -1;
  }
  
  // Process the disk request
  struct disk_request *req = schedule_next_request();
  if(req) {
    process_disk_request(req);
    kfree((void *)req);
  }
  
  // Mark swap entry as no longer in use
  acquire(&swap_disk_lock);
  swap_disk_table[swap_index].in_use = 0;
  swap_disk_table[swap_index].owner = 0;
  swap_disk_table[swap_index].vaddr = 0;
  swap_disk_table[swap_index].logical_block = -1;
  release(&swap_disk_lock);
  
  // Map the new page
  if(mappages(p->pagetable, va, PGSIZE, (uint64)new_pa, PTE_W|PTE_U|PTE_R) != 0) {
    kfree(new_pa);
    return -1;
  }
  
  // Update frame table
  update_frame_table(p, va, new_pa);
  
  // Update statistics
  p->pages_swapped_in++;
  p->resident_pages++;
  
  return 0;
}
