# PA2 SC-MLFQ Scheduler Test Programs

User programs to validate the System-Call-Aware Multi-Level Feedback Queue scheduler.

## Programs

| Program | Description |
|---------|-------------|
| `cpubound` | CPU-bound process (busy loop, minimal syscalls). Should demote to lower queues (ΔS < ΔT). |
| `syscall` | Syscall-heavy process (many getsyscount/getlevel per tick). Should stay at higher queues (ΔS ≥ ΔT). |
| `mixed` | Spawns both CPU-bound and syscall-heavy children, reports MLFQ info after ~20 ticks. |
| `boost` | Runs CPU-bound work, then waits for global priority boost (every 128 ticks) and verifies level resets to 0. |

## How to Run

From the xv6 shell:

```sh
# CPU-bound: should show level > 0 and ticks at lower levels
pa2_tests/cpubound

# Syscall-heavy: should show level 0 or 1, most ticks at level 0
pa2_tests/syscall

# Mixed workload
pa2_tests/mixed 2 2

# Global boost test
pa2_tests/boost
```

## Expected Behavior

- **cpubound**: After running, `level` should be 1–3 and `ticks[1]`, `ticks[2]`, `ticks[3]` should be non-zero.
- **syscall**: `level` should remain 0 or 1; `ticks[0]` should dominate.
- **mixed**: CPU-bound children should have higher levels than syscall-heavy children.
- **boost**: After waiting past a 128-tick boundary, `level` should be 0.
