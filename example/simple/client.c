#include <microkit.h> 
#include <libmicrokitco.h>
#include <printf.h>

#define COSTACK_SIZE 0x1000

co_control_t co_control_mem;

// These should actually be in the system description file with guard page
// but setvar_addr does not work in the x86 SDK currently so we putting them here
char stack1[COSTACK_SIZE];
char stack2[COSTACK_SIZE];
char stack3[COSTACK_SIZE];

void co_entry() {
    void *our_id = microkit_cothread_my_arg();
    printf("CO%ld: hello world\n", our_id);
}

void init(void) {
    float f = 12.34;
    double d = 56.7891234;
    
    printf("CLIENT: starting\n");

    printf("CLIENT: float and double is %f and %lf\n", f, d);

    printf("CLIENT: libmicrokitco derived memsize is %lu bytes\n", 
        LIBMICROKITCO_CONTROLLER_SIZE
    );

    microkit_cothread_init(
        &co_control_mem, 
        COSTACK_SIZE,
        stack1, 
        stack2, 
        stack3
    );

    printf("CLIENT: libmicrokitco started\n");
    
    microkit_cothread_yield();

    microkit_cothread_spawn(co_entry, (void *) 1);
    microkit_cothread_spawn(co_entry, (void *) 2);
    microkit_cothread_spawn(co_entry, (void *) 3);

    if (microkit_cothread_spawn(co_entry, (void *) 4) != LIBMICROKITCO_NULL_HANDLE) {
        printf("ERR: was able to spawn more cothreads than allowed\n");
        return;
    }
    printf("CLIENT: cothreads limit test passed\n");
    printf("CLIENT: cothreads spawned\n");

    microkit_cothread_yield();

    printf("CLIENT: 1, 2, 3 exited, spawning 4, 5, 6\n");

    printf("CLIENT: float and double is %f and %lf\n", f, d);

    microkit_cothread_spawn(co_entry, (void *) 4);
    microkit_cothread_spawn(co_entry, (void *) 5);
    microkit_cothread_spawn(co_entry, (void *) 6);

    if (microkit_cothread_spawn(co_entry, (void *) 7) != LIBMICROKITCO_NULL_HANDLE) {
        printf("ERR: was able to spawn more cothreads than allowed\n");
        return;
    }

    microkit_cothread_yield();

    // returns to Microkit event loop for recv'ing notifications when all cothreads finishes.
}

void notified(microkit_channel channel) {

}
