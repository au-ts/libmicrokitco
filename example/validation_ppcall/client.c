#include <microkit.h>
#include <serial_drv/printf.h>
#include "sel4bench.h"

uintptr_t uart_base;

#define PASSES 20
uint64_t result[PASSES];

inline static void run(int nth) {
    sel4bench_reset_counters();
    
    microkit_ppcall(1, microkit_msginfo_new(0, 0));

    result[nth] = sel4bench_get_cycle_count();
}

void init(void) {
    sel4bench_init();
}

void notified(microkit_channel channel) {
    if (channel == 3) {
        sddf_printf_("Starting round trip call benchmark\n");
        for (int i = 0; i < PASSES; i++) {
            run(i);
        }

        sddf_printf_("Result (cycles):\n");
        for (int i = 0; i < PASSES; i++) {
            sddf_printf_("===> %lu\n", result[i]);
        }

    } else {
        sddf_printf_("Received notification from unknown channel %d\n", channel);
    }
    sddf_printf_("FINISHED\n");
}
