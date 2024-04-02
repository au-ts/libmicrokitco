#include <microkit.h>

void init(void) {
}

void notified(microkit_channel channel) {
}

microkit_msginfo protected(microkit_channel ch, microkit_msginfo msginfo) {
    microkit_msginfo msg = microkit_msginfo_new(0, 0);
    return msg;
}
