#include <microkit.h>
#include <printf.h>

#define SERVER_CHANNEL 0

uintptr_t ipc;

void init(void) {
    uintptr_t client_num = (ipc >> 12) & 0xF;

    printf("CLIENT #%ld: starting\n", client_num);

    microkit_notify(SERVER_CHANNEL);
}

void notified(microkit_channel channel) {
}
