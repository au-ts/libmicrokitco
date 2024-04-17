#include <microkit.h>
#include <serial_drv/printf.h>
#include <libmicrokitco.h>
#include "sel4bench.h"

uintptr_t uart_base;
uintptr_t co_mem;
uintptr_t co_stack;

#define PASSES 20
uint64_t result[PASSES];

inline static void run(int nth) {
    sel4bench_reset_counters();

    microkit_notify(1);
    microkit_cothread_wait(1);

    result[nth] = sel4bench_get_cycle_count();
}

size_t runner(void) {
    sddf_printf_("Starting round trip notify-wait-notify benchmark\n");
    for (int i = 0; i < PASSES; i++) {
        run(i);
    }
    sddf_printf_("Result:\n");
    for (int i = 0; i < PASSES; i++) {
        sddf_printf_("===> %lu\n", result[i]);
    }
    sddf_printf_("FINISHED\n");

    return 0;
}

void init(void) {
    sel4bench_init();

    co_err_t err = microkit_cothread_init(co_mem, 0x1000, 1, co_stack);
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
