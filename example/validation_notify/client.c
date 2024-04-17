#include <microkit.h>
#include <serial_drv/printf.h>
#include "sel4bench.h"

uintptr_t uart_base;

#define PASSES 20
uint64_t result[PASSES];
int nth;

void init(void) {
    sel4bench_init();
}

void notified(microkit_channel channel) {
    if (channel == 3) {
        sddf_printf_("Starting round trip notify benchmark\n");

        sel4bench_reset_counters();

        microkit_notify(1);
        
    } else if (channel == 1) {
        result[nth] = sel4bench_get_cycle_count();
        nth += 1;

        if (nth == PASSES) {
            sddf_printf_("Result:\n");
            for (int i = 0; i < PASSES; i++) {
                sddf_printf_("===> %ld\n", result[i]);
            }
            sddf_printf_("FINISHED\n");
        } else {
            sel4bench_reset_counters();

            microkit_notify(1);
        }

    } else {
        sddf_printf_("Received notification from unknown channel %d\n", channel);
    }
}
