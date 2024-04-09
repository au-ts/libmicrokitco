#include <microkit.h>
#include <sel4/benchmark_utilisation_types.h>
#include <serial_drv/printf.h>
#include "util.h"

uintptr_t uart_base;

uint64_t *ipcbuffer;

#define PASSES 10
uint64_t result[PASSES];
int nth;

void init(void) {
    ipcbuffer = (uint64_t*) &(seL4_GetIPCBuffer()->msg[0]);
    nth = 0;
}

void notified(microkit_channel channel) {
    if (channel == 3) {
        sddf_printf_("Starting round trip notify benchmark\n");

        seL4_BenchmarkResetThreadUtilisation(TCB_CAP);
        seL4_BenchmarkResetLog();

        microkit_notify(1);
        
    } else if (channel == 1) {
        seL4_BenchmarkFinalizeLog();
        seL4_BenchmarkGetThreadUtilisation(TCB_CAP);
        result[nth] = ipcbuffer[BENCHMARK_TOTAL_UTILISATION];
        nth += 1;

        if (nth == PASSES) {
            sddf_printf_("Result:\n");
            for (int i = 0; i < PASSES; i++) {
                sddf_printf_("===> %ld\n", result[i]);
            }
            sddf_printf_("FINISHED\n");
        } else {

            seL4_BenchmarkResetThreadUtilisation(TCB_CAP);
            seL4_BenchmarkResetLog();

            microkit_notify(1);
        }

    } else {
        sddf_printf_("Received notification from unknown channel %d\n", channel);
    }
}
