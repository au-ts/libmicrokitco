// A simple implementation of a Redis-like database server capable of 
// storing number-string key-value pairs and handling multiple clients.

#include <microkit.h>
#include <libmicrokitco.h>
#include "string.h"
#include <stdint.h>

#include <printf.h>

#include "protocol.h"

#define CLIENT1_CHANNEL 0
#define CLIENT2_CHANNEL 1
#define CLIENT3_CHANNEL 2

// database
// large buffer storing N_BUCKETS of 4k strings
uintptr_t db;
// length of the string in each bucket (exclusive of null terminator)
int data_len[N_BUCKETS] = {0};

// cothread things
uintptr_t co_mem;
int stack_size = 0x2000;
uintptr_t stack1;
uintptr_t stack2;
uintptr_t stack3;

// shared buffer for data passing
uintptr_t client1_ipc;
uintptr_t client2_ipc;
uintptr_t client3_ipc;

// argument passing structure to worker cothread
typedef struct {
    microkit_channel channel;
    uintptr_t ipc;
} arg_t;

char *bucket_to_db_vaddr(int bucket) {
    return (char *)((bucket * BUCKET_SIZE) + (char *) db);
}

void client_handler() {
    arg_t *arg_ptr;
    microkit_cothread_my_arg(&arg_ptr);

    arg_t my_owned_args = *arg_ptr;
    microkit_channel this_client_channel = my_owned_args.channel;
    char *this_client_ipc = (char *) my_owned_args.ipc;

    printf("SERVER: client #%d cothread: starting with ipc %p\n", this_client_channel, this_client_ipc);

    while (1) {
        microkit_cothread_wait_on_channel(this_client_channel);

        // got a req from the specific client
        char cmd = this_client_ipc[COMMAND_IPC_IDX];
        int bucket = this_client_ipc[COMMAND_BUCKET_IDX];
        char *ipc_data_start = &this_client_ipc[COMMAND_DATA_START_IDX];
        if (cmd == READ_CMD) {
            printf("SERVER: client #%d cothread: executing read cmd on bucket %d...", this_client_channel, bucket);
            strncpy(this_client_ipc, bucket_to_db_vaddr(bucket), data_len[bucket] + 1);
            printf("done.\n");

        } else if (cmd == APPEND_CMD) {
            printf("SERVER: client #%d cothread: executing append cmd on bucket %d...", this_client_channel, bucket);
            int input_len = strnlen(ipc_data_start, APPEND_CMD_MAX_PAYLOAD_LEN);
            int new_bucket_len = data_len[bucket] + input_len;
            int new_bucket_len_with_null = data_len[bucket] + input_len + 1;

            uint16_t *retval = (uint16_t *) this_client_ipc;
            if (new_bucket_len_with_null > BUCKET_SIZE) {
                printf("rejected\n");
                *retval = 0;
            } else {
                strcat(bucket_to_db_vaddr(bucket), ipc_data_start);
                data_len[bucket] = new_bucket_len;
                *retval = new_bucket_len;
                printf("done.\n");
            }
        } else if (cmd == DELETE_CMD) {
            printf("SERVER: client #%d cothread: got delete cmd on bucket %d...", this_client_channel, bucket);
            uint16_t *retval = (uint16_t *) this_client_ipc;
            data_len[bucket] = 0;
            bucket_to_db_vaddr(bucket)[0] = '\0';
            *retval = 1;
            printf("done\n");
        } else {
            printf("SERVER: client #%d cothread: got UNKNOWN cmd\n", this_client_channel);
        }

        microkit_notify(this_client_channel);
    }
}

void init(void) {
    memzero((void *) db, N_BUCKETS * BUCKET_SIZE);

    printf("SERVER: starting\n");

    co_err_t err = microkit_cothread_init((co_control_t *) co_mem, stack_size, stack1, stack2, stack3);
    if (err != co_no_err) {
        printf("SERVER: ERR: cannot init libmicrokitco, err:\n");
        printf("%s\n", microkit_cothread_pretty_error(err));
        microkit_internal_crash(err);
    }
    printf("SERVER: libmicrokitco started\n");

    microkit_cothread_ref_t _handle;

    printf("SERVER: starting first cothread\n");
    arg_t arg1 = { .channel = CLIENT1_CHANNEL, .ipc = client1_ipc };
    err = microkit_cothread_spawn(client_handler, (uintptr_t) &arg1, &_handle);
    if (err != co_no_err) {
        printf("SERVER: ERR: cannot init first cothread, err:\n");
        printf("%s\n", microkit_cothread_pretty_error(err));
        microkit_internal_crash(err);
    }

    // we get back here after the handler thread does the wait()

    printf("SERVER: starting second cothread\n");
    arg_t arg2 = { .channel = CLIENT2_CHANNEL, .ipc = client2_ipc };
    err = microkit_cothread_spawn(client_handler, (uintptr_t) &arg2, &_handle);
    if (err != co_no_err) {
        printf("SERVER: ERR: cannot init second cothread, err:\n");
        printf("%s\n", microkit_cothread_pretty_error(err));
        microkit_internal_crash(err);
    }


    printf("SERVER: starting third cothread\n");
    arg_t arg3 = { .channel = CLIENT3_CHANNEL, .ipc = client3_ipc };
    err = microkit_cothread_spawn(client_handler, (uintptr_t) &arg3, &_handle);
    if (err != co_no_err) {
        printf("SERVER: ERR: cannot init third cothread, err:\n");
        printf("%s\n", microkit_cothread_pretty_error(err));
        microkit_internal_crash(err);
    }

    // Run all the worker cothreads.
    microkit_cothread_yield();

    // returns to Microkit event loop for recv'ing notifications.
    printf("SERVER: init done!\n");
}

void notified(microkit_channel channel) {
    co_err_t err = microkit_cothread_recv_ntfn(channel);

    if (err == co_no_err) {
        // printf("SERVER: notification %u mapped\n", channel);
    } else {
        printf("SERVER: ERR: mapping notification encountered err: ");
        printf("%s\n", microkit_cothread_pretty_error(err));
        microkit_internal_crash(err);
    }
}
