#include <microkit.h>
#include <printf.h>

#define SERVER_CHANNEL 0

uintptr_t ipc;

char words[] = {
    
}

// each client does a different thing in this example:
void client1_tasks(char *ipc) {
    printf("CLIENT #1: starting\n");
}

void client2_tasks(char *ipc) {
    printf("CLIENT #2: starting\n");
}

void client3_tasks(char *ipc) {
    printf("CLIENT #3: starting\n");
}

void client4_tasks(char *ipc) {
    printf("CLIENT #4: starting\n");
}

void (*task[4])(char *) = {
    client1_tasks,
    client2_tasks,
    client3_tasks,
    client4_tasks
};

void init(void) {
    char *our_ipc = (char *) ipc;
    uintptr_t client_num = (ipc >> 12) & 0xF;
    task[client_num - 1](our_ipc);
}

void notified(microkit_channel channel) {
}
