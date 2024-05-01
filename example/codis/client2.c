#include <microkit.h>
#include <printf.h>
#include <libmicrokitco.h>

#include <string.h>
#include <stdint.h>

#include "protocol.h"

#define SERVER_CHANNEL 0

uintptr_t ipc;
uintptr_t co_mem;
uintptr_t co_stack;

int our_client_num;

size_t co_main(void) {
    char *our_ipc = (char *) ipc;

    our_ipc[COMMAND_IPC_IDX] = APPEND_CMD;
    our_ipc[COMMAND_BUCKET_IDX] = 0;
    char *send = &our_ipc[COMMAND_DATA_START_IDX];
    strcpy(send, "systems that come with provable security, ");

    printf("CLIENT #%d: called server for append, waiting...", our_client_num);
    microkit_notify(SERVER_CHANNEL);
    microkit_cothread_wait(SERVER_CHANNEL);
    printf("done, bucket new len is: %d\n", ((uint16_t *) our_ipc)[0]);
    

    our_ipc[COMMAND_IPC_IDX] = APPEND_CMD;
    our_ipc[COMMAND_BUCKET_IDX] = 0;
    strcpy(send, "safety and reliability guarantees. ");

    printf("CLIENT #%d: called server for append, waiting...", our_client_num);
    microkit_notify(SERVER_CHANNEL);
    microkit_cothread_wait(SERVER_CHANNEL);
    printf("done, bucket new len is: %d\n", ((uint16_t *) our_ipc)[0]);

    // cothread get destroyed after we return.
    return 0;
}

void init(void) {
    our_client_num = (ipc >> 12) & 0xF;
    printf("CLIENT #%d: starting...", our_client_num);

    co_err_t co_err = microkit_cothread_init(co_mem, 0x2000, co_stack);
    if (co_err != co_no_err) {
        printf("CLIENT #%d: ERROR: %s\n", our_client_num, microkit_cothread_pretty_error(co_err));
        microkit_internal_crash(co_err);
    }

    microkit_cothread_t _handle;
    co_err = microkit_cothread_spawn(co_main, ready_true, &_handle, 0);
    if (co_err != co_no_err) {
        printf("CLIENT #%d: ERROR: %s\n", our_client_num, microkit_cothread_pretty_error(co_err));
        microkit_internal_crash(co_err);
    }

    printf("done\n");

    microkit_cothread_yield();
}

void notified(microkit_channel channel) {
    co_err_t err = microkit_cothread_recv_ntfn(channel);

    if (err == co_no_err) {
        //printf("CLIENT #%d: notification %u mapped\n", our_client_num, channel);
    } else if (err == co_err_recv_ntfn_no_blocked) {
        printf("CLIENT #%d: received notification from unknown channel: %d\n", our_client_num, channel);
        // You can handle ntfns from other channels here:
    } else {
        printf("CLIENT #%d: ERR: mapping notification encountered err: ", our_client_num);
        printf("%s\n", microkit_cothread_pretty_error(err));
        microkit_internal_crash(err);
    }
}
