#define LIBCO_C
#include "libco.h"
#include "settings.h"

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

void co_panic(void) {
    char *panic_addr = (char *)0;
    *panic_addr = (char)0;
}

// Cothread context memory layout:
// base | ... | <-stack top | lr | sp | fp | ... | d14 | d15 | top
// If stack overflows then behaviour is undefined. It is recommended that you dedicate
// a discrete Microkit Memory Region for each stack with a guard page at base and top.
// So if a stack does overflow it crashes instead of overwriting other data.

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

        0x6D36382F, // stp d15, d14, [x1, -160]
        0x6D37302D, // stp d13, d12, [x1, -144]
        0x6D38282B, // stp d11, d10, [x1, -128]
        0x6D392029, // stp d9, d8, [x1, -112]
        0xA93A6C3C, // stp x28, x27, [x1, -96]
        0xA93B643A, // stp x26, x25, [x1, -80]
        0xA93C5C38, // stp x24, x23, [x1, -64]
        0xA93D5436, // stp x22, x21, [x1, -48]
        0xA93E4C34, // stp x20, x19, [x1, -32]
        0x910003F0, // mov x16,sp
        0xA93F403D, // stp fp, x16, [x1, -16]
        0xF900003E, // str lr, [x1] // save current context "return from co_swap" pc
        0x6D76380F, // ldp d15, d14, [x0, -160]
        0x6D77300D, // ldp d13, d12, [x0, -144]
        0x6D78280B, // ldp d11, d10, [x0, -128]
        0x6D792009, // ldp d9, d8, [x0, -112]
        0xA97A6C1C, // ldp x28, x27, [x0, -96]
        0xA97B641A, // ldp x26, x25, [x0, -80]
        0xA97C5C18, // ldp x24, x23, [x0, -64]
        0xA97D5416, // ldp x22, x21, [x0, -48]
        0xA97E4C14, // ldp x20, x19, [x0, -32]
        0xA97F401D, // ldp fp, x16, [x0, -16]
        0x9100021F, // mov sp, x16
        0xF940001E, // ldr lr, [x0] // load destination pc
        0xD61F03C0, // br lr // jump to destination pc
};

void co_initialize(void) {
    co_swap = (void (*)(cothread_t, cothread_t))co_swap_function;
}

static void co_entrypoint(void) {
    uintptr_t *buffer_top = (uintptr_t *)co_active_handle;
    ((void (*)(void))buffer_top[-x19])();
    co_panic(); /* Panic if cothread_t entrypoint returns */
}

cothread_t co_active(void) {
    return co_active_handle;
}

cothread_t co_derive(void *memory, unsigned int size, void (*entrypoint)(void)) {
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
