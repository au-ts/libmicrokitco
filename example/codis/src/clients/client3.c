#include <microkit.h>
#include <printf.h>
#include <libmicrokitco.h>

#include "string.h"
#include <stdint.h>

#include "protocol.h"

#define SERVER_CHANNEL 0

uintptr_t ipc;
uintptr_t co_mem;
uintptr_t co_stack;

microkit_cothread_sem_t server_async_io_sem;

int our_client_num;

void co_main(void) {
    char *our_ipc = (char *) ipc;
    printf("CLIENT #%d: called server for read, waiting...", our_client_num);
    our_ipc[COMMAND_IPC_IDX] = READ_CMD;
    our_ipc[COMMAND_BUCKET_IDX] = 0;
    microkit_notify(SERVER_CHANNEL);
    microkit_cothread_semaphore_wait(&server_async_io_sem);
    printf("done, data is: %s\n", our_ipc);

    // cothread get destroyed after we return.
}

void init(void) {
    our_client_num = (ipc >> 12) & 0xF;
    printf("CLIENT #%d: starting...", our_client_num);

    stack_ptrs_arg_array_t costacks = { co_stack };
    microkit_cothread_init((co_control_t *) co_mem, 0x2000, costacks);

    if (microkit_cothread_spawn(co_main, 0) == LIBMICROKITCO_NULL_HANDLE) {
        printf("CLIENT: ERR: cannot init worker cothread\n");
        while (1) {}
    }

    microkit_cothread_semaphore_init(&server_async_io_sem);

    printf("done\n");

    microkit_cothread_yield();
}

void notified(microkit_channel channel) {
    if (channel == SERVER_CHANNEL) {
        microkit_cothread_semaphore_signal(&server_async_io_sem);
    }
}