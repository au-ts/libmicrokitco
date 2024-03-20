# Client Programming Model for Microkit

## Problem
<!-- Lifted from the ToR project brief -->
The [seL4 Microkit](https://trustworthy.systems/projects/microkit/) prescribes an event-handler programming model, which is appropriate for implementing OS services, as well as for reactive clients, which are common in real-time systems. However, it is not the right model for clients that are not reactive, but are built around a computation for which OS services are incidental.

Such a (more traditional) programming model can be implemented on top of the existing event-oriented model through a library that provides a synchronous API over the asynchronous event handlers.

## Aim
Design, implementation and performance evaluation of a library that provides a non-reactive (active process) programming model on top of the Microkit API.

## Solution
### Overview
`libmicrokitco` is a cooperative user-land multithreading library with 2-tier scheduling for use within Microkit. In essence, it allow mapping of multiple executing contexts (cothreads) into 1 Protection Domain (PD). Then, cothreads can wait for an incoming notification from a channel, while some cothreads are waiting, another cothread can execute. 

### Scheduling
A cothread can be "prioritised" or not. All ready "prioritised" cothreads are picked to execute in round robin before non-prioritised cothreads get picked on. Cothreads should yield judiciously to ensure other cothreads are not starved. 

When the scheduler is invoked and no cothreads are ready, the scheduler will return to the root PD thread to receive notifications, see `microkit_cothread_recv_ntfn()`.

The scheduler can be overidden at times by the client, see `microkit_cothread_switch()`.

### Memory model
The library expects a large memory region (MR) for it's internal data structures and many small MRs of *equal size* for the individual co-stacks allocated to it. These memory region must only have read and write permissions. See `microkit_cothread_init()`.

### State transition
> ⚠️ Work in progress, subject to change.
A cothread is in 1 distinct state at any given point in time, interaction with the library or external incoming notifications can trigger a state transition as follow:
![state transition diagram](./docs/state_diagram.png)

## Usage
To use `libmicrokitco` in your project, define these in your Makefile:
1. `LIBMICROKITCO_PATH`: path to root of this library,
2. `MICROKIT_SDK`: path to Microkit SDK,
3. `TOOLCHAIN`: your toolchain, e.g. `aarch64-linux-gnu`,
4. `BUILD_DIR`,
5. `BOARD`: one of Microkit's supported board, e.g. `qemu_arm_virt`,
6. `MICROKIT_CONFIG`: one of `debug` or `release`, and
7. `CPU`: one of Microkit's supported CPU, e.g. `cortex-a53`.

> ##### Danger zone
> Define `LIBMICROKITCO_UNSAFE` to skip most pedantic error checking.

Then, add this to your Makefile after the declarations:
```
include $(LIBMICROKITCO_PATH)/Makefile
```

Finally, for any of your object files that uses this library, link it against `$(BUILD_DIR)/libmicrokitco.o`.

## API
### `co_err_t microkit_cothread_init(uintptr_t controller_memory, int co_stack_size, int max_cothreads, ...)`
Initialises the library's internal data structure.

##### Arguments
Expects:
- `controller_memory` points to the base of an MR that is at least:
`(sizeof(co_tcb_t) * (max_cothreads + 1) + (sizeof(microkit_cothread_t) * 3) * (max_cothreads + 1))` bytes large for internal data structures, and
- `co_stack_size` to be >= 0x1000 bytes.
- `max_cothreads` to be >= 1, which is exclusive of the calling thread.

Then, it expect `max_cothreads` of `uintptr_t` that signify where each co-stacks start.

##### Returns
On error:
- `MICROKITCO_ERR_ALREADY_INITIALISED`,
- `MICROKITCO_ERR_INVALID_ARGS`,
- `MICROKITCO_ERR_NOMEM`,
- `MICROKITCO_ERR_OP_FAIL`.

On success:
- `MICROKITCO_NOERR`.

##### Example
Configuration file:
```xml
<system>
    <!-- Define your system here -->
    <memory_region name="co_mem" size="0x2000"/>
    <memory_region name="stack1" size="0x1000"/>
    <memory_region name="stack2" size="0x1000"/>

    <protection_domain name="example">
        <program_image path="example.elf" />

        <map mr="co_mem" vaddr="0x2000000" perms="rw" setvar_vaddr="co_mem" />
        <map mr="stack1" vaddr="0x2004000" perms="rw" setvar_vaddr="stack_1_start" />
        <map mr="stack2" vaddr="0x2006000" perms="rw" setvar_vaddr="stack_2_start" />

    </protection_domain>
</system>

```
Implementation:
```C
#include <microkit.h>
#include <libmicrokitco.h>

int stack_size = 0x1000;
// These are set in the system config file.
uintptr_t co_mem;
// Stacks should have a GUARD PAGE between them!
uintptr_t stack_1_start;
uintptr_t stack_2_start;

void init(void) {
    if (microkit_cothread_init(co_mem, stack_size, 2, stack_1_start, stack_2_start) != MICROKITCO_NOERR) {
        // handle err
    } else {
        // success, spawn cothreads...
    }
}
```

---

### `co_err_t microkit_cothread_spawn(void (*entry)(void), priority_level_t prioritised, ready_status_t ready, microkit_cothread_t *ret)`
Creates a new cothread, but does not switch to it.

##### Arguments
- `entry` points to your cothread's function. Arguments passing are done via global variables.
- `prioritised` indicates which scheduling queue your cothread will be placed into.
- `ready` indicates whether to schedule your cothread for execution. If you pass `ready_true`, the thread will be placed into the appropriate scheduling queue for execution when the calling thread yields. If you pass `ready_false`, you must later call `mark_ready()` for this cothread to be scheduled.
- `*ret` points to a variable in the caller's stack to write the new cothread's handle to.

##### Returns
On error:
- `MICROKITCO_ERR_NOT_INITIALISED`,
- `MICROKITCO_ERR_INVALID_ARGS`,
- `MICROKITCO_ERR_MAX_COTHREADS_REACHED`,
- `MICROKITCO_ERR_OP_FAIL`.

On success:
- `MICROKITCO_NOERR`.

--- 

### `co_err_t microkit_cothread_mark_ready(microkit_cothread_t cothread)`
Marks an initialised but not ready cothread created from `init()` as ready and schedule it, but does not switch to it.

##### Arguments
- `cothread` is the subject cothread handle.

##### Returns
On error:
- `MICROKITCO_ERR_NOT_INITIALISED`,
- `MICROKITCO_ERR_INVALID_HANDLE`,
- `MICROKITCO_ERR_OP_FAIL`.

On success:
- `MICROKITCO_NOERR`.

--- 

### `void microkit_cothread_yield()`
Yield the CPU to another cothread. If there are no other ready cothreads, the caller cothread keeps running.
---

### `co_err_t microkit_cothread_wait(microkit_channel wake_on)`
Blocks the calling cothread on a notification of a specific Microkit channel then yield. If there are no other ready cothreads, control is switched to the root PD thread for receiving notifications.

> ⚠️ Work in progress, subject to change.

##### Returns
On error:
- `MICROKITCO_ERR_NOT_INITIALISED`,
- `MICROKITCO_ERR_INVALID_ARGS`,
- `MICROKITCO_ERR_OP_FAIL`.

On success:
- `MICROKITCO_NOERR`.

---

### `co_err_t microkit_cothread_switch(microkit_cothread_t cothread)`
Explicitly switches to another ready cothread, overriding the scheduler and priority level. A cothread cannot switch to itself. Do not use `yield()`, `wait()` or `switch()` while editing shared data structures to prevent race. 


##### Returns
On error:
- `MICROKITCO_ERR_NOT_INITIALISED`,
- `MICROKITCO_ERR_DEST_NOT_READY`,
- `MICROKITCO_ERR_INVALID_HANDLE`.

On success:
- `MICROKITCO_NOERR`.

---

### `co_err_t microkit_cothread_recv_ntfn(microkit_channel ch)`
Maps an incoming notification to a blocked cothread *then switches* to it. **Call this in your `notified()`**, otherwise, co-threads will never wake up if they blocks.

> ⚠️ Work in progress, subject to change.

##### Returns
On error:
- `MICROKITCO_ERR_NOT_INITIALISED`,
- `MICROKITCO_ERR_OP_FAIL` if no cothread is blocked on this channel.

On success:
- `MICROKITCO_NOERR`.

##### Example
```C
#include <microkit.h>
#include <libmicrokitco.h>

int stack_size;
uintptr_t co_mem;
uintptr_t stack_1_start;

microkit_channel patron_signal = 42;

void waiter() {
    while (1) {
        microkit_dbg_puts("Waiter: waiting...\n");

        // will switch back to root PD thread for recv'ing notification if no other ready cothread.
        microkit_cothread_wait(patron_signal);

        microkit_dbg_puts("Waiter: coming!\n");
        microkit_cothread_yield();
    }
}

void init(void) {
    microkit_cothread_init(co_mem, stack_size, 1, stack_1_start);
    microkit_cothread_t co_handle;
    microkit_cothread_spawn(waiter, 1, 1, &co_handle);
    microkit_cothread_yield();
}

void notified(microkit_channel ch) {
    if (microkit_cothread_recv_ntfn(ch) != MICROKITCO_NOERR) {
        // No cothread blocked, handle the unknown notification...
    } else {
        // map successful -> switched -> when we get run again, go back to Microkit's event loop.
        return;
    }
}
```

---

### `void microkit_cothread_destroy_me()`
Destroy the calling cothread and invoke the scheduler. **IMPORTANT**, your cothread cannot return, it must call `destroy_me()` instead of returning. Otherwise, it is undefined behaviour if your cothread returns.

Not needed if your cothread is in an infinite loop.

No effect if called in a non-cothread context.

---

### `co_err_t microkit_cothread_destroy_specific(microkit_cothread_t cothread)`
Destroy a specific cothread regardless of their running state. Should be sparingly used because cothread might hold resources that needs free'ing.

##### Returns
On error:
- `MICROKITCO_ERR_NOT_INITIALISED`,
- `MICROKITCO_ERR_INVALID_HANDLE`,
- `MICROKITCO_ERR_OP_FAIL`.

On success:
- `MICROKITCO_NOERR`.

---

### `co_err_t microkit_cothread_prioritise(microkit_cothread_t subject)`
### `co_err_t microkit_cothread_deprioritise(microkit_cothread_t subject)`

Select which scheduling queue the `subject` cothread will be placed into in the future. No immediate effect if the cothread is already scheduled.

##### Returns
On error:
- `MICROKITCO_ERR_NOT_INITIALISED`,
- `MICROKITCO_ERR_INVALID_HANDLE`,

On success:
- `MICROKITCO_NOERR`.

