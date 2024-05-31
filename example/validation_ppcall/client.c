#include <microkit.h>
#include <serial_drv/printf.h>

#if defined(__aarch64__)
    #include "sel4bench_aarch64.h"
#elif defined(__riscv)
    #include "sel4bench_riscv64.h"
#else
    #error "err: unsupported processor, compiler or operating system"
#endif

uintptr_t uart_base;

#define WARMUP_PASSES 8
#define MEASURE_PASSES 32

uint64_t sum_t;
uint64_t sum_sq;
uint64_t result;
uint64_t prev_cycle_count;

static void inline measure(int ith) {
    prev_cycle_count = sel4bench_get_cycle_count();

    microkit_ppcall(1, microkit_msginfo_new(0, 0));

    result = sel4bench_get_cycle_count() - prev_cycle_count;

    sum_t += result;
    sum_sq += result * result;
}

void init(void) {
}

void notified(microkit_channel channel) {
    if (channel == 3) {
        sddf_printf_("Starting round trip call benchmark\n");

        // try to bring everything into cache
        sel4bench_init();
        sel4bench_get_cycle_count();
        sum_t = 0;
        sum_sq = 0;
        result = 0;
        prev_cycle_count = 0;

        for (int i = 0; i < WARMUP_PASSES; i++) {
            microkit_ppcall(1, microkit_msginfo_new(0, 0));
        }
        for (int i = 0; i < MEASURE_PASSES; i++) {
            measure(i);
        }

        sddf_printf_("Mean: %lf\n", sum_t / (float) MEASURE_PASSES);
        sddf_printf_("Stdev = sqrt(%lf)\n", ((MEASURE_PASSES * sum_sq - (sum_t * sum_t)) / (float) (MEASURE_PASSES * (MEASURE_PASSES - 1))));

    } else {
        sddf_printf_("Received notification from unknown channel %d\n", channel);
    }
    sddf_printf_("BENCHFINISHED\n");
}
