#include <microkit.h>
#include <sel4/benchmark_utilisation_types.h>
#include <serial_drv/printf.h>
#include <libmicrokitco.h>
#include "util.h"

uintptr_t uart_base;
uintptr_t co_mem;
uintptr_t co_stack;

uint64_t *ipcbuffer;

#define PASSES 10
uint64_t result[PASSES];

void run(int nth) {
    seL4_BenchmarkResetThreadUtilisation(TCB_CAP);
    seL4_BenchmarkResetLog();

    microkit_notify(1);
    microkit_cothread_wait(1);

    seL4_BenchmarkFinalizeLog();
    seL4_BenchmarkGetThreadUtilisation(TCB_CAP);

    result[nth] = ipcbuffer[BENCHMARK_TOTAL_UTILISATION];
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
    ipcbuffer = (uint64_t*) &(seL4_GetIPCBuffer()->msg[0]);
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
