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

int nth = 0;

void init(void) {
    sel4bench_init();
}

void notified(microkit_channel channel) {
    if (channel == 3) {
        sddf_printf_("Starting round trip notify benchmark\n");

        // try to bring everything we need into cache
        sel4bench_init();
        sel4bench_get_cycle_count();
        sum_t = 0;
        sum_sq = 0;
        result = 0;
        prev_cycle_count = 0;

        microkit_notify(1);
        
    } else if (channel == 1) {
        nth += 1;
        result = sel4bench_get_cycle_count() - prev_cycle_count;

        if (nth > WARMUP_PASSES) {
            sum_t += result;
            sum_sq += result * result;
        }

        if (nth == MEASURE_PASSES + WARMUP_PASSES) {
            sddf_printf_("Mean: %lu\n", sum_t / MEASURE_PASSES);
            sddf_printf_("Stdev = sqrt(%lu)\n", ((MEASURE_PASSES * sum_sq - (sum_t * sum_t)) / (MEASURE_PASSES * (MEASURE_PASSES - 1))));

            sddf_printf_("BENCHFINISHED\n");
        } else {
            prev_cycle_count = sel4bench_get_cycle_count();

            microkit_notify(1);
        }

    } else {
        sddf_printf_("Received notification from unknown channel %d\n", channel);
    }
}
