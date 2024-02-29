#pragma once

#define NULL (void *) 0

void panic() {
    char *panic_addr = (char *) NULL;
    *panic_addr = (char) 0;
}

void memcpy(void *dest, void *source, int size) {
    unsigned char *destination = (unsigned char *) dest;
    for (int i = 0; i < size; i++) {
        unsigned char byte = ((unsigned char *) source)[i];
        destination[i] = byte;
    }
}

void memzero(void *mem, int size) {
    unsigned int zero = 0;
    memcpy(mem, &zero, size);
}
