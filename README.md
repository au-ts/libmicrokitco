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
5. `BOARD`: one Microkit's supported board, e.g. `qemu_arm_virt`,
6. `MICROKIT_CONFIG`: one of `debug` or `release`, and
7. `CPU`: one Microkit's supported CPU, e.g. `cortex-a53`.

##### Danger zone
> Define `LIBMICROKITCO_UNSAFE` to skip most pedantic error checking.

Then, add this to your Makefile:
```
include $(LIBMICROKITCO_PATH)/Makefile
```

And link your object files against `$(BUILD_DIR)/libmicrokitco.o`.

## API
TODO!