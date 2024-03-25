#pragma once

void panic() {
    char *panic_addr = (char *) NULL;
    *panic_addr = (char) 0;
}

void memcpy(void *dest, void *source, int size) {
    unsigned char *destination = (unsigned char *) dest;
    for (int i = 0; i < size; i++) {
        destination[i] = ((unsigned char *) source)[i];
    }
}
