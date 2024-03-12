# Client Programming Model for Microkit

## Overview
A cooperative multithreading library with 2-tier scheduling for use within Microkit. In essence, it allow mapping of multiple executing contexts (cothreads) into 1 protection domain. Then, cothreads can wait for an incoming notification from a channel, while some cothreads are waiting, other cothreads can execute. 

### Scheduling
A cothread can be "prioritised" or not. All ready "prioritised" cothreads are picked to execute in round robin before non-prioritised cothreads get picked on. Cothreads should yield judiciously to ensure other cothreads are not starved. 

When the scheduler is invoked and no cothreads are ready, the scheduler will return to the root PD (Protection Domain) thread to receive notifications, see `microkit_cothread_recv_ntfn()`.

### Memmory model
The library expects a large memory region (MR) for it's internal data structures and many small MRs of *equal size* for the individual co-stacks allocated to it. These memory region must only have read and write permissions. See `microkit_cothread_init()`.

## Usage
To use `libmicrokitco` in your project, define these in your Makefile:
1. `LIBMICROKITCO_PATH`: path to root of this library,
2. `MICROKIT_SDK`: path to Microkit SDK,
3. `TOOLCHAIN`: your toolchain, e.g. `aarch64-linux-gnu`,
4. `BUILD_DIR`,
5. `BOARD`: one of Microkit's supported board, e.g. `qemu_arm_virt`,
6. `MICROKIT_CONFIG`: one of `debug` or `release`, and
7. `CPU`: one of Microkit's supported CPU, e.g. `cortex-a53`.

##### Danger zone
> Define `LIBMICROKITCO_UNSAFE` to skip most pedantic error checking.

Then, add this to your Makefile after the declarations:
```
include $(LIBMICROKITCO_PATH)/Makefile
```

And link your object files against `$(BUILD_DIR)/libmicrokitco.o`.

## API
#### `co_err_t microkit_cothread_init(uintptr_t controller_memory, int co_stack_size, int max_cothreads, ...)`
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
```C
#include <microkit.h>
#include <libmicrokitco.h>

int stack_size;
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

#### `co_err_t microkit_cothread_spawn(void (*entry)(void), int prioritised, int ready, microkit_cothread_t *ret)`
Creates a new cothread, but does not jump to it.

##### Arguments
- `entry` points to your cothread's function. Arguments passing are done via global variables.
- `prioritised` indicates which scheduling queue your cothread will be placed into. Pass non-zero for priority queue and vice versa.
- `ready` indicates whether to schedule your cothread for execution. If you pass non-zero, the thread will be placed into the appropriate scheduling queue for execution when the calling thread yields. If you pass zero, you must later call `mark_ready()` for this cothread to be scheduled.
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

#### `co_err_t microkit_cothread_recv_ntfn(microkit_channel ch)`
Maps an incoming notification to a blocked cothread then switches to it. ***Call this in your `notified()`***, otherwise, co-threads will never wake up if they blocks.

##### Returns
On error:
- `MICROKITCO_ERR_NOT_INITIALISED`,
- `MICROKITCO_ERR_INVALID_ARGS`,
- `MICROKITCO_ERR_OP_FAIL` if no cothread is blocked on this channel.

On success:
- `MICROKITCO_NOERR`.

---