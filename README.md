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
This library supports AArch64, RISC-V (rv64imac) and x86_64.

> The `libco` primitives does support hard-float on RISC-V, but the Microkit is built with soft-float so this entire library is also soft-float for linking.

### State transition

A thread (root or cothread) is in 1 distinct state at any given point in time, interaction with the library or external incoming notifications can trigger a state transition as follow:
![State transition diagram](./docs/state_diagram.png)

## Usage
### Prerequisite
You have two choices of toolchain: LLVM clang or gcc.

For LLVM clang, you need the LLVM toolchain installed and on your machine's `$PATH`:
- `clang`,
- `ld.lld`, and 
- `llvm-objcopy`.

Then define `LLVM = 1` in your Makefile and export it when you invoke libmicrokitco's Makefile.

These `clang` targets have been well tested with this library:
- `aarch64-none-elf`,
- `x86_64-none-elf`,
- `riscv64-none-elf`.

---

For your gcc, define `TOOLCHAIN` in your Makefile. You also need them on your `$PATH`:
- `$(TOOLCHAIN)-gcc`,
- `$(TOOLCHAIN)-ld`,
- `$(TOOLCHAIN)-objcopy` (for x86_64 targets only),

If they are not in your `$PATH`, `$(TOOLCHAIN)` must contain the absolute path to them.

These compiler triples have been well tested with this library:
- `aarch64-unknown-linux-gnu`,
- `aarch64-none-elf`,
- `x86_64-elf`,
- `riscv64-unknown-elf`.


### Compilation
To use `libmicrokitco` in your project, define these in your Makefile:
1. `LIBMICROKITCO_PATH`: path to root of this library,
2. `MICROKIT_SDK`: absolute path to Microkit SDK,
3. `TARGET`: triple, e.g. `aarch64-none-elf`, `x86_64-none-elf`, `riscv64-none-elf`. This is used for naming the output object files and as an argument to LLVM's `clang`.
4. `BUILD_DIR`,
5. `BOARD`: one of Microkit's supported board, e.g. `odroid_c4` or `x86_64_virt`,
6. `MICROKIT_CONFIG`: one of `debug`, `release` or `benchmark`, 
7. `CPU`: one of Microkit's supported CPU, e.g. `cortex-a53`, `nehalem`, or `medany`, 
10. `LIBMICROKITCO_MAX_COTHREADS`: maximum number of cothreads your system needs, and
11. The variables as outlined in Prerequisite.

The compiled object filename will have the form:
```Make
LIBMICROKITCO_OBJ := libmicrokitco_$(LIBMICROKITCO_MAX_COTHREADS)ct_$(TARGET).o
```
For example, a library object file with 5 cothreads configured for AArch64 would have the name:
```Make
libmicrokitco_5ct_aarch64-none-elf.o
```

<!-- ##### Danger zone
> Define `LIBMICROKITCO_UNSAFE` in your preprocessor to skip most pedantic error checking. -->

Then, export those variables and invoke `libmicrokitco`'s Makefile. You could also compile many configurations at once, for example with LLVM:
```Make
TARGET=aarch64-none-elf
LIBMICROKITCO_PATH := ../../
LIBMICROKITCO_1T_OBJ := $(BUILD_DIR)/libmicrokitco/libmicrokitco_1ct_aarch64-none-elf.o
LIBMICROKITCO_3T_OBJ := $(BUILD_DIR)/libmicrokitco/libmicrokitco_3ct_aarch64-none-elf.o

LLVM = 1
export LIBMICROKITCO_PATH TARGET MICROKIT_SDK BUILD_DIR MICROKIT_BOARD MICROKIT_CONFIG CPU LLVM

$(LIBMICROKITCO_1T_OBJ):
	make -f $(LIBMICROKITCO_PATH)/Makefile LIBMICROKITCO_MAX_COTHREADS=1
$(LIBMICROKITCO_3T_OBJ):
	make -f $(LIBMICROKITCO_PATH)/Makefile LIBMICROKITCO_MAX_COTHREADS=3
```

Or with a compiler triple:
```Make
TARGET=aarch64-none-elf
TOOLCHAIN=$(TARGET)
LIBMICROKITCO_PATH := ../../
LIBMICROKITCO_1T_OBJ := $(BUILD_DIR)/libmicrokitco/libmicrokitco_1ct_aarch64-none-elf.o
LIBMICROKITCO_3T_OBJ := $(BUILD_DIR)/libmicrokitco/libmicrokitco_3ct_aarch64-none-elf.o

export LIBMICROKITCO_PATH TARGET MICROKIT_SDK BUILD_DIR MICROKIT_BOARD MICROKIT_CONFIG CPU TOOLCHAIN

$(LIBMICROKITCO_1T_OBJ):
	make -f $(LIBMICROKITCO_PATH)/Makefile LIBMICROKITCO_MAX_COTHREADS=1
$(LIBMICROKITCO_3T_OBJ):
	make -f $(LIBMICROKITCO_PATH)/Makefile LIBMICROKITCO_MAX_COTHREADS=3
```

Finally, for any of your object files that uses this library, link it against `$(LIBMICROKITCO_OBJ)`.


## Foot guns
- If you perform a protected procedure call (PPC), all cothreads in your PD will be blocked even if they are ready until the PPC returns.
- The only time that your PD can receive notifications is when all cothreads are blocked and the scheduler is invoked, then the execution is switched to the root thread where the Microkit event loop runs to receive and dispatch notifications. Consequently, if there is a long running cothread that never blocks, the other cothreads will never wake up if they are blocked on some channel.
- If you have 2 or more cothreads and they use `signal_delayed()`, the previous cothread's signal will get overwritten!


## API
### `const char *microkit_cothread_pretty_error(co_err_t err_num)`
Map the error number returned by this library's functions into a human friendly error message string.

---

### `size_t microkit_cothread_derive_memsize()`
For the compiled configuration, returns the amount of memory the library will needs for it's data structure (excluding the costacks).

---

### `co_err_t microkit_cothread_init(uintptr_t controller_memory_addr, int co_stack_size, ...)`
A variadic function that initialises the library's internal data structure. Each protection domain can only have one "instance" of the library running.

##### Arguments
- `controller_memory_addr` points to the base of an MR that is at least `microkit_cothread_derive_memsize(max_cothreads)` bytes large.
- `co_stack_size` to be >= 0x1000 bytes.

Then, it expect `LIBMICROKITCO_MAX_COTHREADS` (defined at compile time) of `uintptr_t` that point to where each co-stack starts. Giving less than `LIBMICROKITCO_MAX_COTHREADS` is undefined behaviour!

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

### `co_err_t microkit_cothread_recv_ntfn(const microkit_channel ch)`
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
