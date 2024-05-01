#pragma once

void panic() {
    char *panic_addr = (char *) NULL;
    *panic_addr = (char) 0;
}

void co_memcpy(void *dest, void *source, int size) {
    for (int i = 0; i < size; i++) {
        ((unsigned char *) dest)[i] = ((unsigned char *) source)[i];
    }
}
