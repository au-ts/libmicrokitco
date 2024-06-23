#define LIBCO_C
#include "libco.h"
#include "settings.h"

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

void co_panic() {
    char *panic_addr = (char *)0;
    *panic_addr = (char)0;
}

enum {
    lr, // x30
    sp,
    fp,
    x19,
    x20,
    x21,
    x22,
    x23,
    x24,
    x25,
    x26,
    x27,
    x28,
    d8,
    d9,
    d10,
    d11,
    d12,
    d13,
    d14,
    d15,
    client_entry,
    regs_count
};

static thread_local uintptr_t co_active_buffer[regs_count] = { 0 };
static thread_local cothread_t co_active_handle = &co_active_buffer[regs_count - 1];
// co_swap(char *to, char *from)
static void (*co_swap)(cothread_t, cothread_t) = 0;

section(text)
    const uint32_t co_swap_function[] = {

        // x31 (SP): Stack pointer or a zero register, depending on context.
        // x30 (LR): Procedure link register, used to return from subroutines.
        // x29 (FP): Frame pointer.
        // x19 to x28: Callee-saved.
        // x18 (PR): Platform register. Used for some operating-system-specific special purpose, or an additional caller-saved register.
        // x16 (IP0) and x17 (IP1): Intra-Procedure-call scratch registers.
        // x9 to x15: Local variables, caller saved.
        // x8 (XR): Indirect return value address.
        // x0 to x7: Argument values passed to and results returned from a subroutine.

        0xF900003E, // str lr, [x1]
        0x910003F0, // mov x16,sp
        0xF81F8030, // str x16, [x1, -8]
        0xF81F003D, // str fp, [x1, -16]
        0xF81E8033, // str x19, [x1, -24]
        0xF81E0034, // str x20, [x1, -32]
        0xF81D8035, // str x21, [x1, -40]
        0xF81D0036, // str x22, [x1, -48]
        0xF81C8037, // str x23, [x1, -56]
        0xF81C0038, // str x24, [x1, -64]
        0xF81B8039, // str x25, [x1, -72]
        0xF81B003A, // str x26, [x1, -80]
        0xF81A803B, // str x27, [x1, -88]
        0xF81A003C, // str x28, [x1, -96]
        0xFC198028, // str d8, [x1, -104]
        0xFC190029, // str d9, [x1, -112]
        0xFC18802A, // str d10, [x1, -120]
        0xFC18002B, // str d11, [x1, -128]
        0xFC17802C, // str d12, [x1, -136]
        0xFC17002D, // str d13, [x1, -144]
        0xFC16802E, // str d14, [x1, -152]
        0xFC16002F, // str d15, [x1, -160]

        0xF940001E, // ldr lr, [x0]
        0xF85F8010, // ldr x16, [x0, -8]
        0x9100021F, // mov sp, x16
        0xF85F001D, // ldr fp, [x0, -16]
        0xF85E8013, // ldr x19, [x0, -24]
        0xF85E0014, // ldr x20, [x0, -32]
        0xF85D8015, // ldr x21, [x0, -40]
        0xF85D0016, // ldr x22, [x0, -48]
        0xF85C8017, // ldr x23, [x0, -56]
        0xF85C0018, // ldr x24, [x0, -64]
        0xF85B8019, // ldr x25, [x0, -72]
        0xF85B001A, // ldr x26, [x0, -80]
        0xF85A801B, // ldr x27, [x0, -88]
        0xF85A001C, // ldr x28, [x0, -96]
        0xFC598008, // ldr d8, [x0, -104]
        0xFC590009, // ldr d9, [x0, -112]
        0xFC58800A, // ldr d10, [x0, -120]
        0xFC58000B, // ldr d11, [x0, -128]
        0xFC57800C, // ldr d12, [x0, -136]
        0xFC57000D, // ldr d13, [x0, -144]
        0xFC56800E, // ldr d14, [x0, -152]
        0xFC56000F, // ldr d15, [x0, -160]

        0xD61F03C0, // br lr
};

static void co_entrypoint(cothread_t handle) {
    uintptr_t *buffer_top = (uintptr_t *)handle;
    void (*entrypoint)(void) = (void (*)(void))buffer_top[-x19];
    entrypoint();
    co_panic(); /* Panic if cothread_t entrypoint returns */
}

cothread_t co_active() {
    return co_active_handle;
}

cothread_t co_derive(void *memory, unsigned int size, void (*entrypoint)(void)) {
    if (!co_swap)
        co_swap = (void (*)(cothread_t, cothread_t))co_swap_function;

    // We chop up the memory into an array of words.
    uintptr_t *co_local_storage_bottom = (uintptr_t *)memory;
    size_t num_words_storable = size / sizeof(uintptr_t);
    uintptr_t *co_local_storage_top = &co_local_storage_bottom[num_words_storable - 1]; // inclusive

    // Reserve the top num_saved words for registers saves. Then come the stack
    uintptr_t *unaligned_sp = &co_local_storage_bottom[num_words_storable - regs_count - 1];

    // 16-bit align "down" the stack ptr
    uintptr_t aligned_sp = (uintptr_t) unaligned_sp & ~0xF;

    co_local_storage_top[-sp] = aligned_sp;
    co_local_storage_top[-lr] = (uintptr_t)co_entrypoint;
    co_local_storage_top[-x19] = (uintptr_t)entrypoint;
    co_local_storage_top[-fp] = aligned_sp;

    return co_local_storage_top;
}

void co_switch(cothread_t handle) {
    cothread_t co_previous_handle = co_active_handle;
    co_active_handle = handle;
    co_swap(handle, co_previous_handle);
}

#ifdef __cplusplus
}
#endif
