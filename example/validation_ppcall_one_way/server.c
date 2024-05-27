#include <microkit.h>
#include <serial_drv/printf.h>

// #if defined(__aarch64__)
//     #include "sel4bench_aarch64.h"
// #elif defined(__riscv)
//     #include "sel4bench_riscv64.h"
// #else
//     #error "err: unsupported processor, compiler or operating system"
// #endif

uintptr_t uart_base;

void init(void) {
    sddf_printf_("SERVER: started\n");
}

void notified(microkit_channel channel) {
    sddf_printf_("hit noti\n");
}

microkit_msginfo protected(microkit_channel ch, microkit_msginfo msginfo) {
    uint64_t one_way_result = 0; //sel4bench_get_cycle_count();
    microkit_mr_set(0, one_way_result);
    return microkit_msginfo_new(0, 1);
}