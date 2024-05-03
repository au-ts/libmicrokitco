#include <microkit.h>
#include <serial_drv/printf.h>
#include "sel4bench.h"

uintptr_t uart_base;

void init(void) {
    sddf_printf_("SERVER: started\n");
}

void notified(microkit_channel channel) {
    sddf_printf_("hit noti\n");
}

microkit_msginfo protected(microkit_channel ch, microkit_msginfo msginfo) {
    uint64_t one_way_result = sel4bench_get_cycle_count();
    microkit_mr_set(0, one_way_result);
    return microkit_msginfo_new(0, 1);
}
