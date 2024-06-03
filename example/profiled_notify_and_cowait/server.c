#include <microkit.h>
// #include <serial_drv/printf.h>
// #include "util.h"

uintptr_t uart_base;

void init(void) {
    // sddf_printf_("SERVER: started\n");
}

void notified(microkit_channel channel) {
    //sddf_printf_("hit noti chn %d\n", channel);
    microkit_notify(channel);
}

microkit_msginfo protected(microkit_channel ch, microkit_msginfo msginfo) {
    return microkit_msginfo_new(0, 0);
}
