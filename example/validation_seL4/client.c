#include <microkit.h>
#include <sel4/benchmark_utilisation_types.h>
#include <printf.h>

void init(void) {
    printf("CLIENT: sending\n");
    microkit_msginfo msg = microkit_msginfo_new(0, 0);

    uint64_t *ipcbuffer = (uint64_t*) &(seL4_GetIPCBuffer()->msg[0]);
    seL4_BenchmarkResetThreadUtilisation(seL4_CapInitThreadTCB);
    seL4_BenchmarkResetLog();

    microkit_ppcall(1, msg);

    seL4_BenchmarkFinalizeLog();
    seL4_BenchmarkGetThreadUtilisation(seL4_CapInitThreadTCB);

    printf("CLIENT: recved\n");

    printf("Init thread utilisation = %lun", ipcbuffer[BENCHMARK_TCB_UTILISATION]);
    printf("Idle thread utilisation = %lun", ipcbuffer[BENCHMARK_IDLE_TCBCPU_UTILISATION]);
    printf("Overall utilisation = %lun", ipcbuffer[BENCHMARK_TOTAL_UTILISATION]);
}

void notified(microkit_channel channel) {
}
