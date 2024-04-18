# Client Programming Model for Microkit

## Problem
<!-- Lifted from the ToR project brief -->
The [seL4 Microkit](https://trustworthy.systems/projects/microkit/) prescribes an event-handler programming model, which is appropriate for implementing OS services, as well as for reactive clients, which are common in real-time systems. However, it is not the right model for clients that are not reactive, but are built around a computation for which OS services are incidental.

Such a (more traditional) programming model can be implemented on top of the existing event-oriented model through a library that provides a synchronous API over the asynchronous event handlers.

## Aim
Design, implementation and performance evaluation of a library that provides a non-reactive (active process) programming model on top of the Microkit API.

## Solution
### Terminology
- Root Protection Domain (PD) thread: the kernel thread created by seL4 for a protection domain. This is where the Microkit's event loop and entry points runs. For brevity, we will refer to this as "root thread".
- Root TCB: the thread control block of the root thread within this library used to store it's execution context for cothread switching and blocking state.
- Cothread: an execution context in userland that the seL4 kernel is not aware of. The client provides memory for the stack in the form of a Memory Region (MR) with guard page.
- Cothread TCB: serves the same purpose as root TCB, but also stores the virtual address of the cothread's stack.


### Overview
`libmicrokitco` is a cooperative user-land multithreading library with a queue-based scheduler for use within Microkit. In essence, it allow mapping of multiple threads onto one kernel thread of a PD. Then, one or more threads can wait (block) for an incoming notification from a channel or another thread to return, while some threads are blocked, another thread can execute. 

### Scheduling
All ready threads are placed in a queue, the thread at the front will be resumed by the scheduler when it is invoked. Threads should yield judiciously during long running computation to ensure other threads are not starved of CPU time. 

In cases where the scheduler is invoked and no cothreads are ready, the scheduler will return to the root thread to receive notifications, see `microkit_cothread_recv_ntfn()`. Thus, systems adopting this library will not be reactive since notifications are only received when all cothreads are blocked.

### Memory model
The library expects a large memory region (MR) for it's internal data structures and many small MRs of *equal size* for the individual co-stacks allocated to it. These memory regions must only have read and write permissions. See `microkit_cothread_init()`.

### Architecture support
This library supports AArch32, AArch64 and x86_64.

### State transition

TODO: add join() after concrete implementation

A thread (root or cothread) is in 1 distinct state at any given point in time, interaction with the library or external incoming notifications can trigger a state transition as follow:
![state transition diagram](./docs/state_diagram.png)

## Usage
To use `libmicrokitco` in your project, define these in your Makefile:
1. `LIBMICROKITCO_PATH`: absolute path to root of this library,
2. `MICROKIT_SDK`: absolute path to Microkit SDK,
3. `TOOLCHAIN`: your toolchain, e.g. `aarch64-none-elf`,
4. `BUILD_DIR`,
5. `BOARD`: one of Microkit's supported board, e.g. `odroid_c4`,
6. `MICROKIT_CONFIG`: one of `debug`, `release` or `benchmark`, and
7. `CPU`: one of Microkit's supported CPU, e.g. `cortex-a53`.

##### Danger zone
> Define `LIBMICROKITCO_UNSAFE` in your preprocessor to skip most pedantic error checking.

Then, add this to your Makefile after the declarations:
```
include $(LIBMICROKITCO_PATH)/Makefile
```

Finally, for any of your object files that uses this library, link it against `$(BUILD_DIR)/libmicrokitco/libmicrokitco.o`.


## Foot guns
- If you perform a protected procedure call (PPC), all cothreads in your PD will be blocked even if they are ready until the PPC returns.
- The only time that your PD can receive notifications is when all cothreads are blocked and the scheduler is invoked, then the execution is switched to the root thread where the Microkit event loop runs to receive and dispatch notifications. Consequently, if there is a long running cothread that never blocks, the other cothreads will never wake up if they are blocked on some channel.


## API
### `const char *microkit_cothread_pretty_error(co_err_t err_num)`
Map the error number returned by this library's functions into a human friendly error message string.

---

### `size_t microkit_cothread_derive_memsize(int max_cothreads)`

##### Arguments
`max_cothreads` indicates how many cothreads the client intends to have. Exclusive of root thread.

Returns an unsigned integer indicating how much memory this library will need in bytes.

---

### `co_err_t microkit_cothread_init(uintptr_t controller_memory, int co_stack_size, int max_cothreads, ...)`
A variadic function that initialises the library's internal data structure. Each protection domain can only have one "instance" of the library running.

##### Arguments
- `controller_memory` points to the base of an MR that is at least `microkit_cothread_derive_memsize(max_cothreads)` bytes large.
- `co_stack_size` to be >= 0x1000 bytes.
- `max_cothreads` to be >= 1, exclusive of the calling thread.

Then, it expect `max_cothreads` of `uintptr_t` that point to where each co-stack starts.

---

### `co_err_t microkit_cothread_spawn(client_entry_t client_entry, ready_status_t ready, microkit_cothread_t *ret, int num_args, ...);`
A variadic function that creates a new cothread, but does not switch to it.

##### Arguments
- `client_entry` points to your cothread's entrypoint function of the form `size_t (*)(void)`.
- `ready` indicates whether to schedule your cothread for execution. If you pass `ready_true`, the thread will be placed into the scheduling queue for execution when the calling thread yields or blocks. If you pass `ready_false`, you must later call `mark_ready()` for this cothread to be scheduled.
- `*ret` points to a variable in the caller's stack to write the new cothread's handle to.
- `num_args` indicates how many arguments you are passing into the cothreads, maximum 4 arguments of `size_t`.
- `num_args` arguments.

--- 

### `co_err_t microkit_cothread_get_arg(int nth, size_t *ret)`
Fetch the argument of the calling cothread, returns error if called from the root thread.

##### Arguments
- `nth` argument to fetch.
- `*ret` points to a variable in the caller's stack to write the argument to.

---

### `co_err_t microkit_cothread_mark_ready(microkit_cothread_t cothread)`
Marks an initialised but not ready cothread as ready and schedule it, but does not switch to it.

##### Arguments
- `cothread` is the subject cothread handle.

--- 

### `void microkit_cothread_yield()`

Yield the kernel thread to another cothread and place the caller at the back of the scheduling queue. If there are no other ready cothreads, the caller cothread keeps running.

---

### `co_err_t microkit_cothread_wait(microkit_channel wake_on)`
Blocks the calling cothread on a notification of a specific Microkit channel then yield. If there are no other ready cothreads, control is switched to the root PD thread for receiving notifications. Many cothreads can block on the same channel.

##### Arguments
- `wake_on` channel number. Make sure this channel is known to the PD, otherwise, the calling cothreads will block forever.

---

### `co_err_t microkit_cothread_recv_ntfn(microkit_channel ch)`
Maps an incoming notification to blocked cothreads, schedule them then yields to let the newly ready cothreads execute. **Call this in your `notified()`**, otherwise, co-threads will never wake up if they blocks.

This will always runs in the context of the root PD thread.

##### Arguments
- `ch` number that the notification came from.

---

### `co_err_t microkit_cothread_destroy_specific(microkit_cothread_t cothread)`
Destroy a specific cothread regardless of their running state. Should be sparingly used because cothread might hold resources that needs free'ing.

##### Arguments
- `cothread` is the subject cothread handle.

---

### `co_err_t microkit_cothread_join(microkit_cothread_t cothread, size_t *retval)`
Blocks the caller until the #`cothread` thread returns, then write it's return value to `retval`. This API is able to detect simple deadlock scenario such as #1 joins #2, #2 joins #3 then #3 joins #1. A thread cannot join itself. 

Take special care when joining in the root PD thread as you will not be able to receive notifications from seL4.

##### Arguments
- `cothread` is the subject cothread handle.
- `retval` points to a variable on the caller's stack to write the cothread return value to.
