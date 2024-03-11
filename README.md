# Client Programming Model for Microkit

## Description
A cooperative multithreading library with 2-tier scheduling for use within Microkit. In essence, it allow mapping of multiple executing contexts (cothreads) into 1 protection domain. Then, each cothread can wait for an incoming notification from a channel, while it is waiting, other cothreads can execute. 

A cothread can be "prioritised" or not. All ready "prioritised" cothreads are picked to execute in round robin before non-prioritised cothreads get picked on. Cothreads should yield judiciously to ensure other cothreads are not starved. The library expects many memory regions (MR) allocated to it, see microkit_cothread_init().

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
Initialise the library's internal data structure.
##### Arguments
Expects:
- `controller_memory` points to the base of an MR that is at least:
`(sizeof(co_tcb_t) * max_cothreads + (sizeof(microkit_cothread_t) * 3) * max_cothreads)` bytes large for internal data structures, and
- `max_cothreads` to be > 1 as it is inclusive of the calling thread. By default, the calling thread will be prioritised in scheduling.

Then, it expect `max_cothreads - 1` of `uintptr_t` that signify where each co-stacks start.

##### Example
```C
#include <microkit.h>
#include <libmicrokitco.h>

// These are set in the system config file.
uintptr_t co_mem;
uintptr_t stack_size;
// Stacks should have a GUARD PAGE between them, before the first stack and after the last stack!
uintptr_t stack_1_start;
uintptr_t stack_2_start;

void init(void) {
    if (microkit_cothread_init(co_mem, 3, stack_1_left, stack_1_right, stack_2_left, stack_2_right) != MICROKITCO_NOERR) {
        // handle err
    } else {
        // success
    }
}
```