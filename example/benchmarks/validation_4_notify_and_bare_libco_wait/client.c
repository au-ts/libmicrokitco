#include <microkit.h>
#include <serial_drv/printf.h>
#include <libco/libco.h>

#if defined(__aarch64__)
    #include "sel4bench_aarch64.h"
#elif defined(__riscv)
    #include "sel4bench_riscv64.h"
#else
    #error "err: unsupported processor, compiler or operating system"
#endif

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

static void FASTFN measure() {
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
        measure();
    }

    sddf_printf_("Result:\n");

    sddf_printf_("Mean: %lu\n", sum_t / MEASURE_PASSES);
    sddf_printf_("Stdev = sqrt(%lu)\n", ((MEASURE_PASSES * sum_sq - (sum_t * sum_t)) / (MEASURE_PASSES * (MEASURE_PASSES - 1))));

    sddf_printf_("BENCHFINISHED\n");

    co_switch(root_handle);
}

void init(void) {
    root_handle = co_active();
    co_handle = co_derive((void *) co_stack, 0x1000, runner);
}

void notified(microkit_channel channel) {
    switch (channel) {
        case 1:
            co_switch(co_handle);
            break;

        case 3:
            sel4bench_init();
            sel4bench_get_cycle_count();
            sum_t = 0;
            sum_sq = 0;
            result = 0;
            prev_cycle_count = 0;

            co_switch(co_handle);
    }
}
