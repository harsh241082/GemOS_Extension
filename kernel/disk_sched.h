// Disk scheduling, RAID, and swap disk management for PA4

#ifndef DISK_SCHED_H
#define DISK_SCHED_H

#include "types.h"
#include "proc.h"
#include "spinlock.h"

// Disk scheduling policies
#define DISK_SCHED_FCFS 0
#define DISK_SCHED_SSTF 1

// RAID configurations
#define RAID_MODE_0 0  // Striping
#define RAID_MODE_1 1  // Mirroring
#define RAID_MODE_5 2  // Striping with Parity

// Disk and RAID parameters
#define NUM_DISKS 4
#define DISK_BLOCKS_PER_DISK 4096  // Each disk has 4096 blocks
#define SWAP_START_BLOCK 0
#define SWAP_BLOCKS_PER_PAGE 1  // Each 4KB page uses 1 block
#define DISK_ROTATIONAL_DELAY 5  // Constant C in latency model

// Disk request types
#define DISK_READ 0
#define DISK_WRITE 1

// Disk request structure
struct disk_request {
  int type;              // DISK_READ or DISK_WRITE
  int logical_block;     // Logical block number
  int disk_id;           // Which disk (0-3)
  int block_on_disk;     // Block number on that disk
  void *buffer;          // Data buffer for read/write
  struct proc *proc;     // Process requesting this (for priority)
  int priority;          // MLFQ level (lower = higher priority)
  uint64 timestamp;      // When request was queued
  int raid_mode;         // Which RAID mode this request uses
  struct disk_request *next;  // For queue
};

// Global disk state
extern int current_disk_head[NUM_DISKS];  // Current head position for each disk
extern int disk_sched_policy;              // Current scheduling policy
extern int raid_mode;                      // Current RAID configuration

// Statistics
extern uint64 disk_reads;
extern uint64 disk_writes;
extern uint64 total_disk_latency;
extern uint64 disk_operations;

// Function declarations
void disk_sched_init(void);
void disk_queue_init(void);
int add_disk_request(int type, int logical_block, void *buffer, struct proc *p);
struct disk_request* schedule_next_request(void);
int setdisksched(int policy);
void set_raid_mode(int mode);

// RAID mapping functions
int raid_map_logical_to_physical(int logical_block, int *out_disk, int *out_block);
int raid_reverse_map(int disk, int block, int *out_logical);
void raid_stripe_page(void *data, int logical_block, int *disks, int *blocks, int *count);
void raid_reconstruct_page(void *data, int logical_block, int *available_disks);

// Disk I/O functions
uint64 calculate_disk_latency(int disk, int requested_block);
void process_disk_request(struct disk_request *req);

// Statistics
void update_disk_stats(struct disk_request *req, uint64 latency);
uint64 get_avg_disk_latency(void);

#endif // DISK_SCHED_H
