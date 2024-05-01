#pragma once

void panic() {
    char *panic_addr = (char *) NULL;
    *panic_addr = (char) 0;
}
