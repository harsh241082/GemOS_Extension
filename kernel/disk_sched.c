// Disk scheduling and RAID implementation

#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"
#include "disk_sched.h"

// Global disk state
int current_disk_head[NUM_DISKS];
int disk_sched_policy = DISK_SCHED_FCFS;
int raid_mode = RAID_MODE_0;

// Disk request queue
struct disk_request *disk_queue_head = 0;
struct disk_request *disk_queue_tail = 0;
struct spinlock disk_queue_lock;

// Statistics
uint64 disk_reads = 0;
uint64 disk_writes = 0;
uint64 total_disk_latency = 0;
uint64 disk_operations = 0;
struct spinlock disk_stats_lock;

// Initialize disk scheduling system
void
disk_sched_init(void)
{
  initlock(&disk_queue_lock, "disk_queue_lock");
  initlock(&disk_stats_lock, "disk_stats_lock");
  
  // Initialize disk head positions
  for(int i = 0; i < NUM_DISKS; i++) {
    current_disk_head[i] = 0;
  }
  
  disk_sched_policy = DISK_SCHED_FCFS;
  raid_mode = RAID_MODE_0;
}

// Initialize disk request queue
void
disk_queue_init(void)
{
  acquire(&disk_queue_lock);
  disk_queue_head = 0;
  disk_queue_tail = 0;
  release(&disk_queue_lock);
}

// Set disk scheduling policy
int
setdisksched(int policy)
{
  if(policy == DISK_SCHED_FCFS || policy == DISK_SCHED_SSTF) {
    disk_sched_policy = policy;
    return 0;
  }
  return -1;
}

// Set RAID mode (for testing)
void
set_raid_mode(int mode)
{
  if(mode >= RAID_MODE_0 && mode <= RAID_MODE_5) {
    raid_mode = mode;
  }
}

// RAID 0: Striping - distribute blocks across disks
// Logical block b maps to: disk = b % N, block = b / N
int
raid0_map(int logical_block, int *out_disk, int *out_block)
{
  *out_disk = logical_block % NUM_DISKS;
  *out_block = logical_block / NUM_DISKS;
  
  // Verify block is within bounds
  if(*out_block >= DISK_BLOCKS_PER_DISK)
    return -1;
  
  return 0;
}

// RAID 1: Mirroring - store on 2 disks
int
raid1_map(int logical_block, int *out_disk, int *out_block)
{
  // Block stored on two disks: disk 0 and disk 1
  // For simplicity: primary on (logical_block % (NUM_DISKS/2))
  // mirror on disk NUM_DISKS/2 + (logical_block % (NUM_DISKS/2))
  int stripe = logical_block / (DISK_BLOCKS_PER_DISK / 2);
  int block_offset = logical_block % (DISK_BLOCKS_PER_DISK / 2);
  
  if(stripe % 2 == 0) {
    *out_disk = 0;
  } else {
    *out_disk = 2;
  }
  *out_block = block_offset;
  
  if(*out_block >= DISK_BLOCKS_PER_DISK)
    return -1;
  
  return 0;
}

// RAID 5: Striping with Parity
// Parity disk = block_number % NUM_DISKS
int
raid5_map(int logical_block, int *out_disk, int *out_block)
{
  int stripe = logical_block / (NUM_DISKS - 1);
  int block_in_stripe = logical_block % (NUM_DISKS - 1);
  
  // Data blocks are on disks 0 to NUM_DISKS-2
  int data_disk = block_in_stripe;
  
  // Parity disk rotates per stripe
  int parity_disk = stripe % NUM_DISKS;
  
  // Map data block (adjust if it would collide with parity)
  if(data_disk >= parity_disk) {
    data_disk++;
  }
  
  *out_disk = data_disk;
  *out_block = stripe * (DISK_BLOCKS_PER_DISK / NUM_DISKS);
  
  if(*out_block >= DISK_BLOCKS_PER_DISK)
    return -1;
  
  return 0;
}

// Universal RAID mapping function
int
raid_map_logical_to_physical(int logical_block, int *out_disk, int *out_block)
{
  switch(raid_mode) {
    case RAID_MODE_0:
      return raid0_map(logical_block, out_disk, out_block);
    case RAID_MODE_1:
      return raid1_map(logical_block, out_disk, out_block);
    case RAID_MODE_5:
      return raid5_map(logical_block, out_disk, out_block);
    default:
      return -1;
  }
}

// Reverse map: given disk and block, find logical block
int
raid_reverse_map(int disk, int block, int *out_logical)
{
  switch(raid_mode) {
    case RAID_MODE_0: {
      // disk = logical_block % NUM_DISKS
      // block = logical_block / NUM_DISKS
      // So: logical_block = block * NUM_DISKS + disk
      *out_logical = block * NUM_DISKS + disk;
      return 0;
    }
    case RAID_MODE_1: {
      // Primary: disk 0 or 2, block is direct
      *out_logical = block + (disk / 2) * (DISK_BLOCKS_PER_DISK / 2);
      return 0;
    }
    case RAID_MODE_5: {
      // More complex reverse mapping
      int blocks_per_stripe = DISK_BLOCKS_PER_DISK / NUM_DISKS;
      int stripe = block / blocks_per_stripe;
      *out_logical = stripe * (NUM_DISKS - 1) + disk;
      return 0;
    }
    default:
      return -1;
  }
}

// Calculate disk latency: |current_head - requested_block| + C
uint64
calculate_disk_latency(int disk, int requested_block)
{
  int seek_distance = current_disk_head[disk] - requested_block;
  if(seek_distance < 0)
    seek_distance = -seek_distance;
  
  return seek_distance + DISK_ROTATIONAL_DELAY;
}

// Add a disk request to the queue
int
add_disk_request(int type, int logical_block, void *buffer, struct proc *p)
{
  // Allocate new disk request structure
  struct disk_request *req = (struct disk_request *)kalloc();
  if(!req)
    return -1;
  
  // Map logical block to physical location using RAID
  int disk_id, block_on_disk;
  if(raid_map_logical_to_physical(logical_block, &disk_id, &block_on_disk) != 0) {
    kfree((void *)req);
    return -1;
  }
  
  req->type = type;
  req->logical_block = logical_block;
  req->disk_id = disk_id;
  req->block_on_disk = block_on_disk;
  req->buffer = buffer;
  req->proc = p;
  req->priority = p ? p->level : MLFQ_LEVELS - 1;
  req->timestamp = ticks;
  req->raid_mode = raid_mode;
  req->next = 0;
  
  // Add to queue
  acquire(&disk_queue_lock);
  if(disk_queue_tail == 0) {
    disk_queue_head = disk_queue_tail = req;
  } else {
    disk_queue_tail->next = req;
    disk_queue_tail = req;
  }
  release(&disk_queue_lock);
  
  return 0;
}

// FCFS scheduling - return first request in queue
struct disk_request*
schedule_fcfs(void)
{
  acquire(&disk_queue_lock);
  struct disk_request *req = disk_queue_head;
  if(req) {
    disk_queue_head = req->next;
    if(!disk_queue_head)
      disk_queue_tail = 0;
  }
  release(&disk_queue_lock);
  return req;
}

// SSTF scheduling - Shortest Seek Time First
struct disk_request*
schedule_sstf(void)
{
  acquire(&disk_queue_lock);
  
  if(!disk_queue_head) {
    release(&disk_queue_lock);
    return 0;
  }
  
  // Find request with minimum seek distance
  struct disk_request *best = disk_queue_head;
  struct disk_request *prev_best = 0;
  struct disk_request *curr = disk_queue_head;
  struct disk_request *prev_curr = 0;
  
  uint64 min_latency = calculate_disk_latency(best->disk_id, best->block_on_disk);
  
  while(curr) {
    uint64 latency = calculate_disk_latency(curr->disk_id, curr->block_on_disk);
    
    // Prefer higher priority processes (lower level number) on ties
    if(latency < min_latency || 
       (latency == min_latency && curr->priority < best->priority)) {
      min_latency = latency;
      best = curr;
      prev_best = prev_curr;
    }
    
    prev_curr = curr;
    curr = curr->next;
  }
  
  // Remove from queue
  if(prev_best == 0) {
    disk_queue_head = best->next;
  } else {
    prev_best->next = best->next;
  }
  
  if(!disk_queue_head)
    disk_queue_tail = 0;
  
  release(&disk_queue_lock);
  
  best->next = 0;
  return best;
}

// Schedule next disk request based on policy
struct disk_request*
schedule_next_request(void)
{
  if(disk_sched_policy == DISK_SCHED_FCFS) {
    return schedule_fcfs();
  } else if(disk_sched_policy == DISK_SCHED_SSTF) {
    return schedule_sstf();
  }
  return 0;
}

// Process a disk request (simulate the I/O)
void
process_disk_request(struct disk_request *req)
{
  if(!req)
    return;
  
  // Calculate latency
  uint64 latency = calculate_disk_latency(req->disk_id, req->block_on_disk);
  
  // For now, just simulate the latency in statistics
  // In a real system, this would actually trigger disk I/O hardware
  
  // Update disk head position
  current_disk_head[req->disk_id] = req->block_on_disk;
  
  // Update statistics
  update_disk_stats(req, latency);
}

// Update disk statistics
void
update_disk_stats(struct disk_request *req, uint64 latency)
{
  acquire(&disk_stats_lock);
  
  if(req->type == DISK_READ) {
    disk_reads++;
  } else {
    disk_writes++;
  }
  
  total_disk_latency += latency;
  disk_operations++;
  
  // Update per-process statistics if applicable
  if(req->proc) {
    if(req->type == DISK_READ) {
      req->proc->pages_swapped_in++;
    } else {
      req->proc->pages_swapped_out++;
    }
  }
  
  release(&disk_stats_lock);
}

// Get average disk latency
uint64
get_avg_disk_latency(void)
{
  acquire(&disk_stats_lock);
  uint64 avg = 0;
  if(disk_operations > 0) {
    avg = total_disk_latency / disk_operations;
  }
  release(&disk_stats_lock);
  return avg;
}

// RAID helper functions for full file operations

// Stripe a page across multiple disks (for RAID 0/5)
// Maps a logical block to the corresponding physical disk and block positions
void
raid_stripe_page(void *data, int logical_block, int *disks, int *blocks, int *count)
{
  // For RAID 0 and RAID 5, a single logical block maps to one physical location
  // RAID 0: disk = logical_block % NUM_DISKS, block = logical_block / NUM_DISKS
  // RAID 5: distributes data across NUM_DISKS-1 disks with parity on rotating disk
  
  if(!disks || !blocks || !count)
    return;
  
  int disk, block;
  if(raid_map_logical_to_physical(logical_block, &disk, &block) == 0) {
    disks[0] = disk;
    blocks[0] = block;
    *count = 1;
  } else {
    *count = 0;
  }
}

// Reconstruct a page (important for RAID 5 after disk failure)
// Uses XOR parity to recover lost data when one disk is unavailable
void
raid_reconstruct_page(void *data, int logical_block, int *available_disks)
{
  // In RAID 5, if one disk fails, data can be reconstructed using XOR parity
  // The reconstruction formula: Data_i = Parity XOR (Data_0 XOR ... XOR Data_n excluding Data_i)
  // 
  // For PA4 implementation:
  // - Assume no actual disk failures in normal operation
  // - This function provides the API for potential future fault tolerance
  // - In practice, we read from available disks and reconstruct if needed
  //
  // Note: Full XOR parity reconstruction would require reading all data blocks
  // and computing parity values. This is deferred to a full RAID 5 implementation.
  
  if(!data || !available_disks)
    return;
  
  // Placeholder: in a full RAID 5 implementation, this would:
  // 1. Identify which disk contains parity for this logical block
  // 2. Read parity block and all available data blocks
  // 3. XOR them together to recover the missing block
  // 4. Write recovered data to the provided buffer
  //
  // For now, assume all disks are available (no actual failures)
}
