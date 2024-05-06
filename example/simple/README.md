A simple toy example showing `libmicrokitco` in action.

Supports running on AArch64 or x86_64 QEMU systems.

To run AArch64, you need:
- `aarch64-none-elf` toolchain, and
- `qemu-system-aarch64`.
Then invoke the Makefile as `make run_qemu_aarch64 MICROKIT_SDK=/abspath/to/your/SDK`.

To run x86_64, you need:
- `x86_64-elf` toolchain, 
- `qemu-system-x86_64`, and
- A Microkit SDK supporting x86_64.
Then invoke the Makefile as `make run_qemu_x86_64 MICROKIT_SDK=/abspath/to/your/SDK`.
