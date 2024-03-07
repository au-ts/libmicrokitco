#include <microkit.h>
#include <libmicrokitco.h>

uintptr_t co_mem;

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
    microkit_dbg_puts("CO4: hi\n");
    microkit_cothread_destroy_me();
}
void co_entry5() {
    microkit_dbg_puts("CO5: hi\n");
    microkit_cothread_destroy_me();
}

void init(void) {
    microkit_dbg_puts("CLIENT: starting\n");
    
    if (microkit_cothread_init((void *) co_mem, 0x40000, 6) != MICROKITCO_NOERR) {
        microkit_dbg_puts("ERR: cannot init libmicrokitco\n");
        return;
    }
    microkit_dbg_puts("CLIENT: libmicrokitco started\n");

    int co1 = microkit_cothread_spawn(co_entry1, 0, 1);
    int co2 = microkit_cothread_spawn(co_entry2, 0, 1);
    int co3 = microkit_cothread_spawn(co_entry3, 0, 1);
    int co4 = microkit_cothread_spawn(co_entry4, 0, 1);
    int co5 = microkit_cothread_spawn(co_entry5, 0, 1);

    microkit_dbg_puts("CLIENT: cothreads spawned\n");

    // should print the `COn: hi` 5x.
    microkit_cothread_yield();

    // This MUST print before the cothreads' prints because client is higher prio
    microkit_dbg_puts("CLIENT: done, exiting!\n");
}

void notified(microkit_channel channel) {

}
