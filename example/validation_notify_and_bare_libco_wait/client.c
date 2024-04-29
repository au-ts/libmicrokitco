#include <microkit.h>
#include <serial_drv/printf.h>
#include <libco/libco.h>
#include "sel4bench.h"

uintptr_t uart_base;
uintptr_t co_stack;

cothread_t root_handle;
cothread_t co_handle;

#define WARMUP_PASSES 8
#define MEASURE_PASSES 32

uint64_t sum_t;
uint64_t sum_sq;
uint64_t result;
uint64_t prev_cycle_count;

static void FASTFN run() {
    microkit_notify(1);
    co_switch(root_handle);
}

static void FASTFN measure(int ith) {
    prev_cycle_count = sel4bench_get_cycle_count();

    run();

    result = sel4bench_get_cycle_count() - prev_cycle_count;

    sum_t += result;
    sum_sq += result * result;
}

void runner(void) {
    sddf_printf_("Starting round trip notify - wait with bare libco - notify benchmark\n");
    for (int i = 0; i < WARMUP_PASSES; i++) {
        run();
    }
    for (int i = 0; i < MEASURE_PASSES; i++) {
        measure(i);
    }

    sddf_printf_("Result:\n");

    sddf_printf_("Mean: %lf\n", sum_t / (float) MEASURE_PASSES);
    sddf_printf_("Stdev = sqrt(%lf)\n", ((MEASURE_PASSES * sum_sq - (sum_t * sum_t)) / (float) (MEASURE_PASSES * (MEASURE_PASSES - 1))));

    sddf_printf_("FINISHED\n");

    co_switch(root_handle);
}

void init(void) {
    root_handle = co_active();
    co_handle = co_derive((void *) co_stack, 0x1000, runner);
}

void notified(microkit_channel channel) {
    if (channel == 1) {
        co_switch(co_handle);
    } else if (channel == 3) {
        // Start
        
        sel4bench_init();
        sel4bench_get_cycle_count();
        sum_t = 0;
        sum_sq = 0;
        result = 0;
        prev_cycle_count = 0;

        co_switch(co_handle);
    }

}
