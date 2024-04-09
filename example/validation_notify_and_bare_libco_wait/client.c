#include <microkit.h>
#include <sel4/benchmark_utilisation_types.h>
#include <serial_drv/printf.h>
#include <libco/libco.h>
#include "util.h"

uintptr_t uart_base;
uintptr_t co_stack;

cothread_t root_handle;
cothread_t co_handle;

uint64_t *ipcbuffer;

#define PASSES 10
uint64_t result[PASSES];

void run(int nth) {
    seL4_BenchmarkResetThreadUtilisation(TCB_CAP);
    seL4_BenchmarkResetLog();

    microkit_notify(1);
    co_switch(root_handle);

    seL4_BenchmarkFinalizeLog();
    seL4_BenchmarkGetThreadUtilisation(TCB_CAP);

    result[nth] = ipcbuffer[BENCHMARK_TOTAL_UTILISATION];
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
    ipcbuffer = (uint64_t*) &(seL4_GetIPCBuffer()->msg[0]);
    root_handle = co_active();
    co_handle = co_derive((void *) co_stack, 0x1000, runner);

    co_switch(co_handle);
}

void notified(microkit_channel channel) {
    if (channel == 1) {
        co_switch(co_handle);
    }
}
