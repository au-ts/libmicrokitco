#include <microkit.h>
#include <libmicrokitco.h>
#include <printf.h>

uintptr_t co_mem;
int stack_size = 0x1000;
uintptr_t stack1;
uintptr_t stack2;
uintptr_t stack3;

void co_entry() {
    size_t our_id;
    microkit_cothread_get_arg(0, &our_id);
    printf("CO%ld: hi\n", our_id);
}

void init(void) {
    printf("CLIENT: starting\n");

    co_err_t err = microkit_cothread_init(co_mem, stack_size, 3, stack1, stack2, stack3);
    if (err != co_no_err) {
        printf("ERR: cannot init libmicrokitco\n");
        return;
    }
    printf("CLIENT: libmicrokitco started\n");

    microkit_cothread_t co1, co2, co3, co4;
    
    microkit_cothread_spawn(co_entry, ready_true, &co1, 1, 1);
    microkit_cothread_spawn(co_entry, ready_true, &co2, 1, 2);
    microkit_cothread_spawn(co_entry, ready_true, &co3, 1, 3);

    if (microkit_cothread_spawn(co_entry, ready_true, &co4, 1, 4) == co_no_err) {
        printf("ERR: was able to spawn more cothreads than allowed\n");
        return;
    }

    printf("CLIENT: cothreads spawned\n");

    microkit_cothread_yield();

    // This prints before the cothreads' prints because root thread is higher priority.
    printf("CLIENT: done, exiting!\n");

    // returns to Microkit event loop for recv'ing notifications.
}

void notified(microkit_channel channel) {

}
