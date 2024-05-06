#include <microkit.h>
#include <libmicrokitco.h>
#include <printf.h>

#define COMEM_3CT_SIZE 984
#define COSTACK_SIZE 0x1000

char co_mem[COMEM_3CT_SIZE];

// These should actually be in the system description file with guard page
// but setvar_addr does not work in the x86 SDK currently so we putting them here
char stack1[COSTACK_SIZE];
char stack2[COSTACK_SIZE];
char stack3[COSTACK_SIZE];

size_t co_entry() {
    size_t our_id;
    microkit_cothread_get_arg(0, &our_id);
    printf("CO%ld: hello world\n", our_id);
    return 0;
}

void init(void) {
    printf("CLIENT: starting\n");

    printf("CLIENT: libmicrokitco derived memsize is %lu bytes\n", 
        microkit_cothread_derive_memsize()
    );
    if (COMEM_3CT_SIZE < microkit_cothread_derive_memsize()) {
        printf("CLIENT: ERROR: but we only allocated %d bytes!\n", COMEM_3CT_SIZE);
        microkit_internal_crash(0);
    }

    co_err_t err = microkit_cothread_init(
        co_mem, 
        COSTACK_SIZE,
        stack1, 
        stack2, 
        stack3
    );
    if (err != co_no_err) {
        printf("CLIENT: ERR: cannot init libmicrokitco, err is %s", microkit_cothread_pretty_error(err));
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
