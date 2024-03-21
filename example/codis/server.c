#include <microkit.h>
#include <libmicrokitco.h>

#include <printf.h>

#define CLIENT1_CHANNEL 1
#define CLIENT2_CHANNEL 2
#define CLIENT3_CHANNEL 3
#define CLIENT4_CHANNEL 4

uintptr_t co_mem;
int stack_size = 0x2000;
uintptr_t stack1;
uintptr_t stack2;
uintptr_t stack3;
uintptr_t stack4;

unsigned int arg_pass;

uintptr_t client1_ipc;
uintptr_t client2_ipc;
uintptr_t client3_ipc;
uintptr_t client4_ipc;

void client_handler() {
    microkit_channel client_channel = arg_pass;
    printf("SERVER: client #%d cothread: starting\n", client_channel);

    while (1) {
        microkit_cothread_wait(client_channel);
        printf("SERVER: client #%d cothread: got ntfn!\n", client_channel);
    }

    // there must be a `destroy_me()` here if we dont have an infinite loop.
}

void init(void) {
    printf("SERVER: starting\n");

    co_err_t err = microkit_cothread_init(co_mem, stack_size, 4, stack1, stack2, stack3, stack4);
    if (err != MICROKITCO_NOERR) {
        printf("SERVER: ERR: cannot init libmicrokitco, err:\n");
        printf("%s\n", microkit_cothread_pretty_error(err));
        microkit_internal_crash(0);
    }
    printf("SERVER: libmicrokitco started\n");

    microkit_cothread_t _handle;

    printf("SERVER: starting first cothread\n");
    arg_pass = CLIENT1_CHANNEL;
    err = microkit_cothread_spawn(client_handler, priority_false, ready_true, &_handle);
    if (err != MICROKITCO_NOERR) {
        printf("SERVER: ERR: cannot init first cothread, err:\n");
        printf("%s\n", microkit_cothread_pretty_error(err));
        microkit_internal_crash(0);
    }

    // we deprioritise ourselves to let the cothread have a chance to execute.
    microkit_cothread_deprioritise(MICROKITCO_ROOT_THREAD);
    // client handler thread #1 now execute.
    microkit_cothread_yield();


    printf("SERVER: starting second cothread\n");
    arg_pass = CLIENT2_CHANNEL;
    err = microkit_cothread_spawn(client_handler, priority_false, ready_true, &_handle);
    if (err != MICROKITCO_NOERR) {
        printf("SERVER: ERR: cannot init second cothread, err:\n");
        printf("%s\n", microkit_cothread_pretty_error(err));
        microkit_internal_crash(0);
    }
    // client handler thread #2 now execute.
    microkit_cothread_yield();


    printf("SERVER: starting third cothread\n");
    arg_pass = CLIENT3_CHANNEL;
    err = microkit_cothread_spawn(client_handler, priority_false, ready_true, &_handle);
    if (err != MICROKITCO_NOERR) {
        printf("SERVER: ERR: cannot init third cothread, err:\n");
        printf("%s\n", microkit_cothread_pretty_error(err));
        microkit_internal_crash(0);
    }
    // client handler thread #3 now execute.
    microkit_cothread_yield();


    printf("SERVER: starting fourth cothread\n");
    arg_pass = CLIENT4_CHANNEL;
    err = microkit_cothread_spawn(client_handler, priority_false, ready_true, &_handle);
    if (err != MICROKITCO_NOERR) {
        printf("SERVER: ERR: cannot init fourth cothread, err:\n");
        printf("%s\n", microkit_cothread_pretty_error(err));
        microkit_internal_crash(0);
    }
    // client handler thread #4 now execute.
    microkit_cothread_yield();

    // nothing happens since all 4 cothreads are waiting for channel ntfn.
    microkit_cothread_yield();

    // returns to Microkit event loop for recv'ing notifications.
    printf("SERVER: init done!\n");


    // Correct print order:

    // SERVER: starting
    // SERVER: libmicrokitco started
    // SERVER: starting first cothread
    // SERVER: client #1 cothread: starting
    // SERVER: starting second cothread
    // SERVER: client #2 cothread: starting
    // SERVER: starting third cothread
    // SERVER: client #3 cothread: starting
    // SERVER: starting fourth cothread
    // SERVER: client #4 cothread: starting
    // SERVER: init done!
}

void notified(microkit_channel channel) {
    co_err_t err = microkit_cothread_recv_ntfn(channel);

    if (err == MICROKITCO_NOERR) {
        ;
    } else if (err == MICROKITCO_ERR_OP_FAIL) {
        printf("SERVER: received notification from unknown channel: %d\n", channel);
        // You can handle ntfns from other channels here:
    } else {
        printf("SERVER: ERR: mapping notification encountered err:\n");
        printf("%s\n", microkit_cothread_pretty_error(err));
        microkit_internal_crash(0);
    }

}
