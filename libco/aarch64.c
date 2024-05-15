#define LIBCO_C
#include "libco.h"
#include "settings.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

inline void panic()
{
    char *panic_addr = (char *)0;
    *panic_addr = (char)0;
}

#define STACK_CANARY 0x341294AA8642FE71

static thread_local uintptr_t co_active_buffer[32];
static thread_local cothread_t co_active_handle = 0;
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

        0x910003f0, /* mov x16,sp           */ // x16 = sp
        0xa9007830, /* stp x16,x30,[x1]     */ // from[0] = x16, from[8] = x30
        0xa9407810, /* ldp x16,x30,[x0]     */ // x16 = to[0], x30 = to[8]
        0x9100021f, /* mov sp,x16           */ // sp = x16
        0xa9015033, /* stp x19,x20,[x1, 16] */ // from[16] = x19, from[24] = x20
        0xa9415013, /* ldp x19,x20,[x0, 16] */ // x19 = to[16], x20 = to[24]
        0xa9025835, /* stp x21,x22,[x1, 32] */ // and so on...
        0xa9425815, /* ldp x21,x22,[x0, 32] */
        0xa9036037, /* stp x23,x24,[x1, 48] */
        0xa9436017, /* ldp x23,x24,[x0, 48] */
        0xa9046839, /* stp x25,x26,[x1, 64] */
        0xa9446819, /* ldp x25,x26,[x0, 64] */
        0xa905703b, /* stp x27,x28,[x1, 80] */
        0xa945701b, /* ldp x27,x28,[x0, 80] */ // end of callee saved
        0xf900303d, /* str x29,    [x1, 96] */ // from[96] = frame pointer
        0xf940301d, /* ldr x29,    [x0, 96] */ // frame pointer = to[96]
        0x6d072428, /* stp d8, d9, [x1,112] */ // from[112] = d8, from[120] = d9
        0x6d472408, /* ldp d8, d9, [x0,112] */ // d8 = to[112], d9 = to[120]
        0x6d082c2a, /* stp d10,d11,[x1,128] */ // and so on...
        0x6d482c0a, /* ldp d10,d11,[x0,128] */
        0x6d09342c, /* stp d12,d13,[x1,144] */
        0x6d49340c, /* ldp d12,d13,[x0,144] */
        0x6d0a3c2e, /* stp d14,d15,[x1,160] */
        0x6d4a3c0e, /* ldp d14,d15,[x0,160] */ // end of floating point register saves
        0xd61f03c0,
        /* br x30               */ // PC = x30 = to[8]
};

static void co_entrypoint(cothread_t handle)
{
    uintptr_t *buffer = (uintptr_t *)handle;
    void (*entrypoint)(void) = (void (*)(void))buffer[2];
    entrypoint();
    panic(); /* Panic if cothread_t entrypoint returns */
}

cothread_t co_active()
{
    if (!co_active_handle)
        co_active_handle = &co_active_buffer;
    return co_active_handle;
}

cothread_t co_derive(void *memory, unsigned int size, void (*entrypoint)(void))
{
    uintptr_t *handle;
    if (!co_swap)
        co_swap = (void (*)(cothread_t, cothread_t))co_swap_function;

    if (!co_active_handle)
        co_active_handle = &co_active_buffer;

    if ((handle = (uintptr_t *)memory))
    {
        unsigned int offset = (size & ~15);
        uintptr_t *p = (uintptr_t *)((unsigned char *)handle + offset);
        handle[0] = (uintptr_t)p;             /* x16 (stack pointer) */
        handle[1] = (uintptr_t)co_entrypoint; /* x30 (link register) */
        handle[2] = (uintptr_t)entrypoint;    /* x19 (entry point) */
        handle[12] = (uintptr_t)p;            /* x29 (frame pointer) */
        handle[21] = STACK_CANARY;
    }

    return handle;
}

void co_switch(cothread_t handle)
{
    uintptr_t *memory = (uintptr_t *)handle;
    if (co_active_buffer[21] != STACK_CANARY || memory[21] != STACK_CANARY)
    {
        panic();
    }
    cothread_t co_previous_handle = co_active_handle;
    co_swap(co_active_handle = handle, co_previous_handle);
}

#ifdef __cplusplus
}
#endif
