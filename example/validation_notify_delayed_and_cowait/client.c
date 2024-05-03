#include <microkit.h>
#include <serial_drv/printf.h>
#include <libmicrokitco.h>
#include "sel4bench.h"

uintptr_t uart_base;
uintptr_t co_mem;
uintptr_t co_stack;

#define WARMUP_PASSES 8
#define MEASURE_PASSES 32

uint64_t sum_t;
uint64_t sum_sq;
uint64_t result;
uint64_t prev_cycle_count;

static void FASTFN run() {
    microkit_notify_delayed(1);
    microkit_cothread_wait(1);
}

static void FASTFN measure(int nth) {
    prev_cycle_count = sel4bench_get_cycle_count();

    run();

    result = sel4bench_get_cycle_count() - prev_cycle_count;

    sum_t += result;
    sum_sq += result * result;
}

size_t runner(void) {
    sddf_printf_("Starting round trip notify-wait-notify benchmark\n");

    sel4bench_init();
    sel4bench_get_cycle_count();
    sum_t = 0;
    sum_sq = 0;
    result = 0;
    prev_cycle_count = 0;

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

    return 0;
}

void init(void) {
    co_err_t err = microkit_cothread_init(co_mem, 0x1000, co_stack);
    if (err != co_no_err) {
        sddf_printf_("CLIENT: Cannot init libmicrokitco, err is :%s\n", microkit_cothread_pretty_error(err));
    } else {
        sddf_printf_("CLIENT: libmicrokitco started\n");
    }

    microkit_cothread_t _handle;
    err = microkit_cothread_spawn(runner, ready_true, &_handle, 0);
    if (err != co_no_err) {
        sddf_printf_("CLIENT: Cannot spawn runner cothread, err is :%s\n", microkit_cothread_pretty_error(err));
    } else {
        sddf_printf_("CLIENT: runner cothread started\n");
    }

    microkit_cothread_yield();
}

void notified(microkit_channel channel) {
    microkit_cothread_recv_ntfn(channel);
}
