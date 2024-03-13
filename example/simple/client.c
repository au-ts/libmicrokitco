#include <microkit.h>
#include <libmicrokitco.h>

uintptr_t co_mem;
int stack_size = 0x1000;
uintptr_t stack1;
uintptr_t stack2;
uintptr_t stack3;

void co_entry1() {
    microkit_dbg_puts("CO1: hi\n");
    microkit_cothread_destroy_me();
}
void co_entry2() {
    microkit_dbg_puts("CO2: hi\n");
    microkit_cothread_destroy_me();
}
void co_entry3() {
    microkit_dbg_puts("CO3: hi\n");
    microkit_cothread_destroy_me();
}

void co_entry4() {
    // should not enter
    *(char *) 0 = 0;
}

void init(void) {
    microkit_dbg_puts("CLIENT: starting\n");

    co_err_t err = microkit_cothread_init(co_mem, stack_size, 3, stack1, stack2, stack3);
    if (err != MICROKITCO_NOERR) {
        microkit_dbg_puts("ERR: cannot init libmicrokitco\n");
        return;
    }
    microkit_dbg_puts("CLIENT: libmicrokitco started\n");

    microkit_cothread_t co1, co2, co3, co4;
    
    microkit_cothread_spawn(co_entry1, 0, 1, &co1);
    microkit_cothread_spawn(co_entry2, 0, 1, &co2);
    microkit_cothread_spawn(co_entry3, 0, 1, &co3);

    if (microkit_cothread_spawn(co_entry4, 0, 1, &co4) != MICROKITCO_ERR_MAX_COTHREADS_REACHED) {
        microkit_dbg_puts("ERR: was able to spawn more cothreads than allowed\n");
        return;
    }

    microkit_dbg_puts("CLIENT: cothreads spawned\n");

    microkit_cothread_yield();

    // This prints before the cothreads' prints because root thread is higher priority.
    microkit_dbg_puts("CLIENT: done, exiting!\n");

    microkit_cothread_deprioritise(0);
    microkit_cothread_yield();

    // returns to Microkit event loop for recv'ing notifications.
}

void notified(microkit_channel channel) {

}
