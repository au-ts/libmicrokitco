#include <microkit.h>
#include <serial_drv/printf.h>
#include <libco/libco.h>
#include "sel4bench.h"

uintptr_t uart_base;
uintptr_t co_stack;

cothread_t root_handle;
cothread_t co_handle;

#define PASSES 20
uint64_t result[PASSES];

inline static void run(int nth) {
    sel4bench_reset_counters();

    microkit_notify(1);
    co_switch(root_handle);

    result[nth] = sel4bench_get_cycle_count();
}

void runner(void) {
    sddf_printf_("Starting round trip notify - wait with bare libco - notify benchmark\n");
    for (int i = 0; i < PASSES; i++) {
        run(i);
    }

    sddf_printf_("Result:\n");
    for (int i = 0; i < PASSES; i++) {
        sddf_printf_("===> %lu\n", result[i]);
    }
    sddf_printf_("FINISHED\n");

    co_switch(root_handle);
}

void init(void) {
    sel4bench_init();

    root_handle = co_active();
    co_handle = co_derive((void *) co_stack, 0x1000, runner);

    co_switch(co_handle);
}

void notified(microkit_channel channel) {
    if (channel == 1) {
        co_switch(co_handle);
    }
}
