# -D LIBMICROKITCO_UNSAFE
aarch64-linux-gnu-gcc -c -mcpu=cortex-a53 -mstrict-align -nostdlib -ffreestanding -g -Wall -Wno-array-bounds -Wno-unused-variable -Wno-unused-function -Werror -I./sdk/board/qemu_arm_virt/debug/include -Iinclude -DBOARD_qemu_arm_virt libmicrokitco.c -o build/libmicrokitco.o
