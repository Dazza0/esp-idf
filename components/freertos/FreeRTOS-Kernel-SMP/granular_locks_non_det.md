# Summary

This document contains implementation notes for SMP FreeRTOS Non-Deterministic Granular Locks. This implementation acts as a follow up to the issues regarding the dead locking issues faced in the previous `granular_locks_proposal.md` (part A) proposal.

# Overview of Non-Deterministic Granular Locking

The Non-Deterministic Granular Locking model makes two major changes to the SMP FreeRTOS kernel.

1. Groups FreeRTOS data into data groups, each protected by its own lock, in order to reduce lock contention (see `granular_locks_proposal.md` for more details regarding data groups).
2. **Removes all determinism requirements** (i.e., walking linked lists while interrupts are disabled is now permitted) in order to improve performance and simplify the granular locking implementation.

# Port Configuration

- `portGRANULAR_LOCKING` (default `0`) will enable/disable granular locking.
  - Disabling granular locking will revert to the original SMP FreeRTOS BKL (Big Kernel Lock) implementation (using a global task lock and ISR lock).
- `portCRITICAL_NESTING_IN_TCB` (default `0`) is also supported with granular locking.
  - `0` means that FreeRTOS will provide granular locking implementations of `...EnterCritica()` functions so that critical nesting can be tracked inside the TCB.
  - `1` means that the port must provide granular locking implementations for `...ENTER_CRITICAL()` so that the port can track critical nesting.

Given the large number of permutations regarding critical section implementations (e.g., single-core/BKL/granular + with/without TCB nesting), `FreeRTOS.h` will define `FREERTOS_CRIT_IMPL` indicating what critical section implementation is currently being used. `FREERTOS_CRIT_IMPL` is then used to `#if`/`#else` throughout the other parts of the kernel's source code. The possible values of `FREERTOS_CRIT_IMPL` are...

- `FREERTOS_CRIT_IMPL = 0`
    - Single core critical sections (i.e., interrupt disable). Critical nesting in TCB
- `FREERTOS_CRIT_IMPL = 1`
    - Single core critical sections (i.e., interrupt disable). Critical nesting in port
- `FREERTOS_CRIT_IMPL = 2`
    - SMP critical sections, big kernel lock (i.e., global task and ISR lock). Critical nesting in TCB
- `FREERTOS_CRIT_IMPL = 3`
    - SMP critical sections, big kernel lock (i.e., global task and ISR lock). Critical nesting in port
- `FREERTOS_CRIT_IMPL = 4`
    - SMP critical sections, granular locking (i.e., lock per data group). Critical nesting in TCB
- `FREERTOS_CRIT_IMPL = 5`
    - SMP critical sections, granular locking (i.e., lock per data group). Critical nesting in port

# Port Macros

## Common

The following macros need to be provided if granular locking is enabled (i.e., `FREERTOS_CRIT_IMPL = 4/5`)

- `portSPINLOCK_TYPE` defining the data type of the spinlock
- `portSPINLOCK_xxx_INIT( pxSpinlock )` or `portSPINLOCK_xxx_INIT_STATIC` macros to initialize the spinlock of a particular data group, where `xxx` is the data group.
- `portTAKE_LOCK( pxLock )`/`portRELEASE_LOCK( pxLock )` to take and release a spinlock

## Granular Locks & TCB Nesting (`FREERTOS_CRIT_IMPL = 4`)

When `FREERTOS_CRIT_IMPL = 4`, the implementation of the critical section functions are provided by FreeRTOS. These functions will track the critical nesting count in the TCB and call `portTAKE_LOCK( pxLock )`/`portRELEASE_LOCK( pxLock )`. FreeRTOS will define the following critical section functions:

- `prvKernelEnterCritical[FromISR]`/`prvKernelExitCritical[FromISR]()` to enter/exit a critical section of the kernel data group.
- `vTaskEnterCriticalWithLock[FromISR]()`/`vTaskExitCriticalWithLock[FromISR]()` to enter/exit a critical section of other data groups.

## Granular Locks & Port Nesting (`FREERTOS_CRIT_IMPL = 5`)

When `FREERTOS_CRIT_IMPL = 5`, the implementation of the critical section functions are provided by port. Thus, the port must provide the following macros.

- `portENTER_CRITICAL_WITH_LOCK( pxLock )`
  - Disables interrupts, take spinlock, increment nesting count
- `portENTER_CRITICAL_WITH_LOCK_FROM_ISR( pxLock )`
  - From ISR version, thus returns previous interrupt state
- `portEXIT_CRITICAL_WITH_LOCK( pxLock )`
  - Decrement nesting count, release spinlock, reenable interrupts
- `portEXIT_CRITICAL_WITH_LOCK_FROM_ISR( pxLock, uxInterruptStatus )`
  - From ISR version, restores previous interrupt state
- Macros to get the critical nesting count
  - `portGET_CRITICAL_NESTING_COUNT()`
  - `portSET_CRITICAL_NESTING_COUNT( x )`
  - `portINCREMENT_CRITICAL_NESTING_COUNT()`
  - `portDECREMENT_CRITICAL_NESTING_COUNT()`
- `portPEND_YIELD_IN_CRIT()` to let the port know that a yield is required when the outermost critical section exits.

# Data Group Specifics

## Kernel

The kernel data group is guarded by the following locks:

```c
PRIVILEGED_DATA static portSPINLOCK_TYPE xKernelTaskLock = portSPINLOCK_KERNEL_TASK_INIT_STATIC;
PRIVILEGED_DATA static portSPINLOCK_TYPE xKernelISRLock = portSPINLOCK_KERNEL_ISR_INIT_STATIC;
```

All critical sections in `tasks.c` now call the following macros to enter/exit a critical section in order to take the kernel data group spinlocks.

```c
kernelENTER_CRITICAL()
kernelEXIT_CRITICAL()
kernelENTER_CRITICAL_FROM_ISR()
kernelEXIT_CRITICAL_FROM_ISR( x )
```

- Task critical sections take both `xKernelTaskLock` and `xKernelISRLock`
- ISR critical sections take only `xKernelISRLock`
- `vTaskSuspendAll()` takes only `xKernelTaskLock`
- The following functions that use `vTaskSuspendAll()`/`xTaskResumeAll()` blocks for deterministic access now use a critical section instead:
  - `xTaskDelayUntil()`
  - `vTaskDelay()`
  - `xTaskGetHandle()`
  - `uxTaskGetSystemState()`
  - `xTaskAbortDelay()`
  - `vTaskGetInfo()`
- Functions to add/remove tasks from task lists now also take/release the kernel lock. These will be called form other data groups inside a nested critical section
  - `vTaskPlaceOnEventList()`
  - `vTaskPlaceOnUnorderedEventList()`
  - `vTaskPlaceOnEventListRestricted()`
  - `xTaskRemoveFromEventList()`
  - `vTaskRemoveFromUnorderedEventList()`

## Queue

Each instantiated queue is considered its own data group, thus each queue is protected by its own spinlock:

```c
portSPINLOCK_TYPE xQueueLock;
```

All critical sections in `queue.c` now call the following macros to enter/exit a critical section in order to take the queue data group's spinlock:

```c
queueENTER_CRITICAL( pxQueue )
queueEXIT_CRITICAL( pxQueue )
queueENTER_CRITICAL_FROM_ISR( pxQueue )
queueEXIT_CRITICAL_FROM_ISR( pxQueue, uxInterruptStatus )
```

Given that `queueENTER_CRITICAL()` and `vTaskSuspendAll()` now target different spinlocks (i.e., `xQueueLock` vs `xKernelTaskLock` respectively), it is possible for two tasks to simultaneously be in a queue critical sections of separate queues.

Functions previously using `vTaskSuspendAll()`/`xTaskResumeAll()` blocks and `cRxLock`/`cTxLock`  for deterministic access now simply use a critical section.

## Event Groups

Each instantiated event group is considered its own data group, thus each event group is protected by its own spinlock:

```c
portSPINLOCK_TYPE xEventGroupLock;
```

All critical sections in `event_groups.c` now call the following macros to enter/exit a critical section in order to take the event group's spinlock:

```c
eventENTER_CRITICAL( pxQueue )
eventEXIT_CRITICAL( pxQueue )
eventENTER_CRITICAL_FROM_ISR( pxQueue )
eventEXIT_CRITICAL_FROM_ISR( pxQueue, uxInterruptStatus )
```

- All `vTaskSuspendAll()`/`xTaskResumeAll()` blocks are now replaced with critical sections.
- `xEventGroupSetBits()`/`vEventGroupDelete()` will attempt to walk the `xTasksWaitingForBits` lists directly inside the function (even though all tasks lists belong to the kernel data group). Therefore, a `vTaskKernelLock()` call is added to take the kernel data groups lock from another data group.


## Stream Buffer

Each instantiated stream buffer is considered its own data group, thus each event group is protected by its own spinlock:

```c
portSPINLOCK_TYPE xStreamBufferLock;
```

All critical sections in `stream_buffer.c` now call the following macros to enter/exit a critical section in order to take the stream buffer's spinlock:

```c
sbENTER_CRITICAL( pxStreamBuffer )
sbEXIT_CRITICAL( pxStreamBuffer )
sbENTER_CRITICAL_FROM_ISR( pxStreamBuffer )
sbEXIT_CRITICAL_FROM_ISR( pxStreamBuffer, uxInterruptStatus )
```

- All `vTaskSuspendAll()`/`xTaskResumeAll()` blocks are now replaced with critical sections.

## Timers

All timers and timer static data are considered a single data group, thus are protected by the same spinlock:

```c
PRIVILEGED_DATA static portSPINLOCK_TYPE xTimerLock = portSPINLOCK_TIMER_INIT_STATIC;
```

All critical sections in `timers.c` now call the following macros to enter/exit a critical section in order to take the timer spinlock:

```c
timerENTER_CRITICAL( pxTimerLock )
timerEXIT_CRITICAL( pxTimerLock )
```

All `vTaskSuspendAll()`/`xTaskResumeAll()` blocks are now replaced with critical sections.

## User

All user data is treated as the same data group, thus are protected by the same spinlock. The kernel will define the following spinlock for user data:

```c
PRIVILEGED_DATA portSPINLOCK_TYPE xUserLock = portSPINLOCK_USER_INIT_STATIC;
```

When user code calls `taskENTER_CRITICAL[_FROM_ISR]()`/`taskEXIT_CRITICAL_[FROM_ISR]()`, the macros are defined to take `xUserLock`.

# Run State Change

Previously, `prvCheckForRunStateChange()` was called in every `taskENTER_CRITICAL()` or `vTaskSuspendAll()` to ensure that pending yields would occur before the `taskENTER_CRITICAL()` or `vTaskSuspendAll()` sections were allowed to proceed. This approach is no longer feasible with granular locks for the following reasons:

- `prvCheckForRunStateChange()` introduces a significant overhead given that critical sections occur frequently.
- In the nested critical that has taken multiple spinlocks, there is not easy way to release then retake the multiple spinlocks in the correct order.

Thus, granular locking makes the following changes to avoid calling `prvCheckForRunStateChange()` in every `taskENTER_CRITICAL()`.

## Checking For Yields in `vTaskSuspendAll()`

Given that `vTaskSuspendAll()` is never called in a critical section, the "nesting across multiple locks" scenario will never occur. Thus, `vTaskSuspendAll()` will now call `prvCheckSuspendAllStateChange()` to check for pending yields. `prvCheckSuspendAllStateChange()` is a simplified version of `prvCheckForRunStateChange()` to account for the fact that it is never called in a critical section or in an ISR.

## Preventing Corruption on Pended Yields

When a core X requests core Y to yield, core Y could already be in a critical section or contesting for one. In most cases, allowing core Y to execute its critical section to completion is not an issue as the behavior would would be equivalent to core Y receive the yield after it exits the critical section. For example:

1. Core X requests core Y to yield
2. Core Y is in `xQueueSend()` contesting for a queue critical section
3. Core Y is allowed to continue executing its critical section and copies data to the queue
4. Core Y exits the critical section and finally handles the yield

The behavior is equivalent to core Y receiving the yield request after it exits the critical section.

Delaying the yield is only an issue if the following conditions are met:

- Core X is in a critical section (e.g., holding spinlock of data group A) when requesting core Core Y to yield
- Core Y is contesting for the same critical section that core X is currently in (e.g., contesting for spinlock of data group A)
- The nature of the changes that Core Y makes to data group A ends up corrupting the changes made by previously by Core X

Given these conditions, delaying the handling of a yield request only becomes an issue if Core X and Core Y both try to change Core Y's state (e.g., suspending or deleting the task on Core Y). Therefore, so long as Core Y can skips its changes to the kernel data group (i.e., moving tasks to a different list) if a yield is pending, then there will be no issue allowing the yield to be handled later. As a result, the following changes are made to `tasks.c`:

- `prvYieldCore()` now specifies a yield reason (`eInternalTaskState`). This lets Core Y know if a yield it is pending (and the yield's reason) once Core Y has entered its critical section
- `vTaskSuspend(task_Y)` is updated to check for pending yields of task Y. If task Y has already been suspended or deleted, `vTaskSuspend()` will not move the task to the suspended list
- `vTaskDelete(task_Y)` is updated to check for pending yields of task Y. If task Y has already been deleted, `vTaskDelete()` will not delete task Y again.

## Code Changes

- Updated `taskTASK_IS_xxx()` macros to get the current yield state of a task (i.e., `eInternalTaskState`). A task could be not running, running, or pending a yield due to one of multiple reasons.
- Added `taskMARK_TASK_IS_xxx()` to update a tasks current yield state
- Updated `vTaskPlaceOnEventList()`/`vTaskPlaceOnUnorderedEventList()` to skip placing a task on an event list if that task is already pending a yield (i.e., has already been placed on a different list or has been deleted).

# Tests and Results

Multiple unit tests have been added to test the performance increase of granular locks compared to BKL. All tests were run on a dual Xtensa 160MHz core ESP32 chip. Granular locking builds were built with `portCRITICAL_NESTING_IN_TCB = 0` and `portGRANULAR_LOCKING = 1`.

## Critical Section Speed

The `crit_speed.c` test measures the speed of the critical section functions themselves (i.e., `taskENTER_CRITICAL()` and `taskEXIT_CRITICAL()`).

Results are as follows (in number of CPU cycles):

- Granular Locking

  ```
  taskENTER_CRITICAL() average elapsed time: 139
  taskEXIT_CRITICAL() average elapsed time: 125
  ```

- BKL

  ```
  taskENTER_CRITICAL() average elapsed time: 299
  taskEXIT_CRITICAL() average elapsed time: 207
  ```

Granular locking has a ≈54% and ≈40% decrease in the elapsed time of `taskENTER_CRITICAL()` and `taskEXIT_CRITICAL()` respectively (likely due to the elimination of `prvCheckForRunStateChange()` and

## Lock Contention End to End

The `lock_contention_end_to_end.c` test measures the speed difference between granular locks and BKL in the scenario where each core handles a separate data stream. Each core has a producer task, consumer task, and a queue to pass data between the tasks.

Results are as follows (in number of CPU cycles):

- Granular Locking

  ```
  Core 0: 820948
  Core 1: 810131
  ```

- BKL

  ```
  Core 0: 1129064
  Core 1: 1129236
  ```

Granular locking shows a ≈28% decrease in run time (likely due to queues now having separate separate locks). However, queue access takes a relatively small amount of time compared to the context switching between the producer and consumer tasks. Thus, queue contention is likely to occur less frequently.

## Queue Contention

The `queue_contention.c` test measures the speed difference between granular locks and BKL when multiple cores simultaneously access separate queues without blocking.

Results are as follows (in number of CPU cycles):

- Granular Locking

  ```
  Core 0: 329629
  Core 1: 329745
  ```

- BKL

  ```
  Core 0: 580804
  Core 1: 583905
  ```

Granular locking shows a ≈43% decrease in run time. Each core rapidly access their own queues, thus lock contention in BKL is frequent.

## Queue Speed

The `queue_speed.c` tests measures the speed of the queue functions themselves (i.e., `xQueueSend()`/`xQueueReceive()`).

Results are as follows (in number of CPU cycles):

Granular Locking

```
xQueueSend() average elapsed time: 1059
xQueueReceive() average elapsed time: 1022
```

BKL

```
xQueueSend() average elapsed time: 1227
xQueueReceive() average elapsed time: 1190
```

Granular locking shows a ≈14% decrease in both `xQueueSend()` and `xQueueReceive()`.