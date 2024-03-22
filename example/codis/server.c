// A simple implementation of a Redis-like database server capable of 
// storing number-string key-value pairs and handling multiple clients.

#include <microkit.h>
#include <libmicrokitco.h>
#include <string.h>
#include <stdint.h>

#include <printf.h>

#define CLIENT1_CHANNEL 1
#define CLIENT2_CHANNEL 2
#define CLIENT3_CHANNEL 3
#define CLIENT4_CHANNEL 4

// database things
#define N_BUCKETS 0xFF // 255
#define BUCKET_SIZE 0x1000
// large buffer storing N_BUCKETS of 4k strings
uintptr_t db;
// length of the string in each bucket
int data_len[N_BUCKETS] = {0};

// database protocol:
// first 8 bits in ipc buff is command number
#define COMMAND_IPC_IDX 0
// next 8 bits is bucket number
#define COMMAND_BUCKET_IDX 1
// the rest is the string input (if applicable)
#define COMMAND_DATA_START_IDX 2

// commands:
#define READ_CMD 0
#define APPEND_CMD 1
#define APPEND_CMD_MAX_PAYLOAD_LEN (BUCKET_SIZE - 2)

#define DELETE_CMD 2

// return:
// cmd 0: string occupy entire ipc buff
// cmd 1: first 16 bits indicate new length of the string in bucket, zero if the 
//        concatenation would cause the string to overflow the bucket.
// cmd 2: first 16 bits is a flag, non zero means the string was deleted successfully.


// cothread things
uintptr_t co_mem;
int stack_size = 0x2000;
uintptr_t stack1;
uintptr_t stack2;
uintptr_t stack3;
uintptr_t stack4;

unsigned int chn_arg_pass;
uintptr_t ipc_arg_pass;

uintptr_t client1_ipc;
uintptr_t client2_ipc;
uintptr_t client3_ipc;
uintptr_t client4_ipc;

char *bucket_to_db_vaddr(int bucket) {
    return (char *)((uint64_t)bucket * BUCKET_SIZE + &db);
}

void client_handler() {
    microkit_channel client_channel = chn_arg_pass;
    char *client_ipc = (char *) ipc_arg_pass;
    printf("SERVER: client #%d cothread: starting with ipc %p\n", client_channel, client_ipc);

    while (1) {
        microkit_cothread_wait(client_channel);

        // got a req from the specific client
        char cmd = client_ipc[COMMAND_IPC_IDX];
        int bucket = client_ipc[COMMAND_BUCKET_IDX];
        char *ipc_data_start = &client_ipc[COMMAND_DATA_START_IDX];
        if (cmd == READ_CMD) {
            printf("SERVER: client #%d cothread: executing read cmd on bucket %d...", client_channel, bucket);
            strncpy(client_ipc, bucket_to_db_vaddr(bucket), data_len[bucket]);
            printf("done.\n");

        } else if (cmd == APPEND_CMD) {
            printf("SERVER: client #%d cothread: executing append cmd on bucket %d...", client_channel, bucket);
            int input_len = strnlen(ipc_data_start, APPEND_CMD_MAX_PAYLOAD_LEN);
            int new_bucket_len = data_len[bucket] + input_len;
            int new_bucket_len_with_null = data_len[bucket] + input_len + 1;

            uint16_t *retval = (uint16_t *) client_ipc;
            if (new_bucket_len_with_null > BUCKET_SIZE) {
                printf("rejected\n");
                *retval = 0;
            } else {
                strcat(bucket_to_db_vaddr(bucket), ipc_data_start);
                data_len[bucket] = new_bucket_len;
                *retval = new_bucket_len;
                printf("done\n");
            }
        } else if (cmd == DELETE_CMD) {
            printf("SERVER: client #%d cothread: got delete cmd on bucket %d...", client_channel, bucket);
            uint16_t *retval = (uint16_t *) client_ipc;
            data_len[bucket] = 0;
            bucket_to_db_vaddr(bucket)[0] = '\0';
            *retval = 1;
            printf("done\n");
        } else {
            printf("SERVER: client #%d cothread: got UNKNOWN cmd\n", client_channel);
        }

        microkit_notify(client_channel);
    }

    // there must be a `destroy_me()` here if we dont have an infinite loop.
}

void init(void) {
    memzero((void *) db, N_BUCKETS * BUCKET_SIZE);

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
    chn_arg_pass = CLIENT1_CHANNEL;
    ipc_arg_pass = client1_ipc;
    err = microkit_cothread_spawn(client_handler, priority_false, ready_true, &_handle);
    if (err != MICROKITCO_NOERR) {
        printf("SERVER: ERR: cannot init first cothread, err:\n");
        printf("%s\n", microkit_cothread_pretty_error(err));
        microkit_internal_crash(0);
    }

    // we deprioritise ourselves to let the cothread have a chance to execute right after spawn for
    // args passing.
    microkit_cothread_deprioritise(MICROKITCO_ROOT_THREAD);
    // client handler thread #1 now execute.
    microkit_cothread_yield();


    printf("SERVER: starting second cothread\n");
    chn_arg_pass = CLIENT2_CHANNEL;
    ipc_arg_pass = client2_ipc;
    err = microkit_cothread_spawn(client_handler, priority_false, ready_true, &_handle);
    if (err != MICROKITCO_NOERR) {
        printf("SERVER: ERR: cannot init second cothread, err:\n");
        printf("%s\n", microkit_cothread_pretty_error(err));
        microkit_internal_crash(0);
    }
    // client handler thread #2 now execute.
    microkit_cothread_yield();


    printf("SERVER: starting third cothread\n");
    chn_arg_pass = CLIENT3_CHANNEL;
    ipc_arg_pass = client3_ipc;
    err = microkit_cothread_spawn(client_handler, priority_false, ready_true, &_handle);
    if (err != MICROKITCO_NOERR) {
        printf("SERVER: ERR: cannot init third cothread, err:\n");
        printf("%s\n", microkit_cothread_pretty_error(err));
        microkit_internal_crash(0);
    }
    // client handler thread #3 now execute.
    microkit_cothread_yield();


    printf("SERVER: starting fourth cothread\n");
    chn_arg_pass = CLIENT4_CHANNEL;
    ipc_arg_pass = client4_ipc;
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
        printf("SERVER: notification %u mapped\n", channel);
    } else if (err == MICROKITCO_ERR_OP_FAIL) {
        printf("SERVER: received notification from unknown channel: %d\n", channel);
        // You can handle ntfns from other channels here:
    } else {
        printf("SERVER: ERR: mapping notification encountered err:\n");
        printf("%s\n", microkit_cothread_pretty_error(err));
        microkit_internal_crash(0);
    }

}
