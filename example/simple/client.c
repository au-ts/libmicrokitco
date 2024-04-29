#include <microkit.h>
#include <libmicrokitco.h>
#include <printf.h>

uintptr_t co_mem;
int stack_size = 0x1000;
uintptr_t stack1;
uintptr_t stack2;
uintptr_t stack3;

size_t co_entry() {
    size_t our_id;
    microkit_cothread_get_arg(0, &our_id);
    printf("CO%ld: hi\n", our_id);
    return 0;
}

void init(void) {
    printf("CLIENT: starting\n");

    printf("CLIENT: libmicrokitco derived memsize is %ld bytes, max_cothreads is %d\n", 
        microkit_cothread_derive_memsize(), 
        microkit_cothread_fetch_defined_num_cothreads()
    );

    co_err_t err = microkit_cothread_init(
        co_mem, 
        stack_size, 
        microkit_cothread_fetch_defined_num_cothreads(), 
        stack1, 
        stack2, 
        stack3
    );
    if (err != co_no_err) {
        printf("ERR: cannot init libmicrokitco, err is %s", microkit_cothread_pretty_error(err));
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
    printf("CLIENT: cothreads limit test passed\n");

    printf("CLIENT: cothreads spawned\n");

    microkit_cothread_yield();

    printf("CLIENT: done, exiting!\n");

    // returns to Microkit event loop for recv'ing notifications when all cothreads finishes.
}

void notified(microkit_channel channel) {

}
