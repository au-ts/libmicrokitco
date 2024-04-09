#include <microkit.h>
#include <sel4/benchmark_utilisation_types.h>
#include <serial_drv/printf.h>
#include "util.h"

uintptr_t uart_base;

uint64_t *ipcbuffer;

#define PASSES 10
uint64_t result[PASSES];

void run(int nth) {
    seL4_BenchmarkResetThreadUtilisation(TCB_CAP);
    seL4_BenchmarkResetLog();

    microkit_ppcall(1, microkit_msginfo_new(0, 0));

    seL4_BenchmarkFinalizeLog();
    seL4_BenchmarkGetThreadUtilisation(TCB_CAP);

    result[nth] = ipcbuffer[BENCHMARK_TOTAL_UTILISATION];
}

void init(void) {
    ipcbuffer = (uint64_t*) &(seL4_GetIPCBuffer()->msg[0]);
}

void notified(microkit_channel channel) {
    if (channel == 3) {
        sddf_printf_("Starting round trip call benchmark\n");
        for (int i = 0; i < PASSES; i++) {
            run(i);
        }

        sddf_printf_("Result:\n");
        uint64_t accumulator = 0;
        for (int i = 0; i < PASSES; i++) {
            sddf_printf_("Pass #%d: %lu cycles\n", i, result[i]);
            accumulator += result[i];
        }
        sddf_printf_("Average: %ld cycles\n", accumulator / PASSES);
        

    } else {
        sddf_printf_("Received notification from unknown channel %d\n", channel);
    }
    sddf_printf_("FINISHED\n");
}
