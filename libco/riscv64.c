/*
 * Copyright 2024, UNSW
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

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
// base | ... | <-stack top | ra | sp | fp | ... | pc | client_entry | top
// If stack overflows then behaviour is undefined. It is recommended that you dedicate
// a discrete Microkit Memory Region for each stack with a guard page at base and top.
// So if a stack does overflow it crashes instead of overwriting other data.

#ifndef __riscv_flen
enum
{
    client_entry,
    pc,
    s11,
    s10,
    s9,
    s8,
    s7,
    s6,
    s5,
    s4,
    s3,
    s2,
    s1,
    fp, // AKA s0
    sp,
    ra, // always NULL
    num_saved
};
#else
enum
{
    client_entry,
    pc,
    f27,
    f26,
    f25,
    f24,
    f23,
    f22,
    f21,
    f20,
    f19,
    f18,
    f9, // and so on...
    f8, // AKA fs0
    s11,
    s10,
    s9,
    s8,
    s7,
    s6,
    s5,
    s4,
    s3,
    s2,
    s1,
    fp, // AKA s0
    sp,
    ra, // always NULL
    num_saved
};
#endif

static thread_local uintptr_t root_cothread_buffer[num_saved] = { 0 };

// All cothread_t will point to the top word in the buffer
static thread_local cothread_t co_active_handle = &root_cothread_buffer[num_saved - 1];

// Quick reference: https://www.cl.cam.ac.uk/teaching/1617/ECAD+Arch/files/docs/RISCVGreenCardv8-20151013.pdf
// Instructions encoded with this tool: https://luplab.gitlab.io/rvcodecjs/

#ifndef __riscv_flen
// Soft float only
section(text)
    const uint32_t co_swap_function[] = {
        // RV64I InsSet
        // Begin saving callee saved registers of current context

        // We first shift the context buffer ptr down 15 words, then saving from there
        0xf885859b, // addiw a1, a1, -120

        0x0015b023, // sd ra, 0(a1)
        0x0025b423, // sd sp, 8(a1)
        0x0085b823, // sd s0, 16(a1)
        0x0095bc23, // sd s1, 24(a1)
        0x0325b023, // sd s2, 32(a1)
        0x0335b423, // sd s3, 40(a1)
        0x0345b823, // sd s4, 48(a1)
        0x0355bc23, // sd s5, 56(a1)
        0x0565b023, // sd s6, 64(a1)
        0x0575b423, // sd s7, 72(a1)
        0x0585b823, // sd s8, 80(a1)
        0x0595bc23, // sd s9, 88(a1)
        0x07a5b023, // sd s10, 96(a1)
        0x07b5b423, // sd s11, 104(a1)

        // When co_swap is called, `ra` have the PC we need to resume the `from` cothread!
        0x0615b823, // sd ra, 112(a1)

        // Begin loading callee saved registers of the context we are switching to
        0xf885051b, // addiw a0, a0, -120

        0x00053083, // ld ra, 0(a0)
        0x00853103, // ld sp, 8(a0)
        0x01053403, // ld s0, 16(a0)
        0x01853483, // ld s1, 24(a0)
        0x02053903, // ld s2, 32(a0)
        0x02853983, // ld s3, 40(a0)
        0x03053a03, // ld s4, 48(a0)
        0x03853a83, // ld s5, 56(a0)
        0x04053b03, // ld s6, 64(a0)
        0x04853b83, // ld s7, 72(a0)
        0x05053c03, // ld s8, 80(a0)
        0x05853c83, // ld s9, 88(a0)
        0x06053d03, // ld s10, 96(a0)
        0x06853d83, // ld s11, 104(a0)

        // load the PC of the destination context then jump to it
        // discard link result
        0x07053603, // ld a2, 112(a0)
        0x00060067, // jalr a2, 0(a2)
};
#else

// Hard float
section(text)
    // This has not been tested due to a lack of hard-float Microkit binary
    const uint32_t co_swap_function[] = {
        // RV64I InsSet
        // Begin saving callee saved registers of current context

        0xf285859b, // addiw a1, a1, -216

        0x0015b023, // sd ra, 0(a1)
        0x0025b423, // sd sp, 8(a1)
        0x0085b823, // sd s0, 16(a1)
        0x0095bc23, // sd s1, 24(a1)
        0x0325b023, // sd s2, 32(a1)
        0x0335b423, // sd s3, 40(a1)
        0x0345b823, // sd s4, 48(a1)
        0x0355bc23, // sd s5, 56(a1)
        0x0565b023, // sd s6, 64(a1)
        0x0575b423, // sd s7, 72(a1)
        0x0585b823, // sd s8, 80(a1)
        0x0595bc23, // sd s9, 88(a1)
        0x07a5b023, // sd s10, 96(a1)
        0x07b5b423, // sd s11, 104(a1)

        0x0685b827, // fsd f8, 112(a1)  EQUIV fsd fs0, 112(a1)
        0x0695bc27, // fsd f9, 120(a1)
        0x0925b027, // fsd f18, 128(a1)
        0x0935b427, // fsd f19, 136(a1)
        0x0945b827, // fsd f20, 144(a1)
        0x0955bc27, // fsd f21, 152(a1)
        0x0b65b027, // fsd f22, 160(a1)
        0x0b75b427, // fsd f23, 168(a1)
        0x0b85b827, // fsd f24, 176(a1)
        0x0b95bc27, // fsd f25, 184(a1)
        0x0da5b027, // fsd f26, 192(a1)
        0x0db5b427, // fsd f27, 200(a1)

        // When co_swap is called, `ra` have the PC we need to resume the `from` cothread!
        0x0c15b823, // sd ra, 208(a1)

        0xf285051b, // addiw a0, a0, -216

        // Begin loading callee saved registers of the context we are switching to
        0x00053083, // ld ra, 0(a0)
        0x00853103, // ld sp, 8(a0)
        0x01053403, // ld s0, 16(a0)
        0x01853483, // ld s1, 24(a0)
        0x02053903, // ld s2, 32(a0)
        0x02853983, // ld s3, 40(a0)
        0x03053a03, // ld s4, 48(a0)
        0x03853a83, // ld s5, 56(a0)
        0x04053b03, // ld s6, 64(a0)
        0x04853b83, // ld s7, 72(a0)
        0x05053c03, // ld s8, 80(a0)
        0x05853c83, // ld s9, 88(a0)
        0x06053d03, // ld s10, 96(a0)
        0x06853d83, // ld s11, 104(a0)

        0x07053407, // fld f8, 112(a0)
        0x07853487, // fld f9, 120(a0)
        0x08053907, // fld f18, 128(a0)
        0x08853987, // fld f19, 136(a0)
        0x09053a07, // fld f20, 144(a0)
        0x09853a87, // fld f21, 152(a0)
        0x0a053b07, // fld f22, 160(a0)
        0x0a853b87, // fld f23, 168(a0)
        0x0b053c07, // fld f24, 176(a0)
        0x0b853c87, // fld f25, 184(a0)
        0x0c053d07, // fld f26, 192(a0)
        0x0c853d87, // fld f27, 200(a0)

        // load the PC of the destination context then jump to it
        // discard link result
        0x0d053603, // ld a2, 208(a0)
        0x00060067, // jalr a2, 0(a2)
};
#endif

static void co_entrypoint(void) {
    uintptr_t *buffer_top = (uintptr_t *)co_active_handle;
    void (*entrypoint)(void) = (void (*)(void))buffer_top[-client_entry];
    entrypoint();
    co_panic(); /* Panic if cothread_t entrypoint returns */
}

static void (*co_swap)(cothread_t, cothread_t) = (void (*)(cothread_t, cothread_t))co_swap_function;

cothread_t co_active(void) {
    return co_active_handle;
}

cothread_t co_derive(void *memory, unsigned int size, void (*entrypoint)(void)) {
    // We chop up the memory into an array of words.
    uintptr_t *co_local_storage_bottom = (uintptr_t *)memory;
    size_t num_words_storable = size / sizeof(uintptr_t);
    uintptr_t *co_local_storage_top = &co_local_storage_bottom[num_words_storable - 1]; // inclusive
 
    // Reserve the top num_saved words for registers saves. Then come the stack
    uintptr_t *unaligned_sp = &co_local_storage_bottom[num_words_storable - num_saved - 1];

    // 16-bit align "down" the stack ptr
    uintptr_t aligned_sp = (uintptr_t) unaligned_sp & ~0xF;

    co_local_storage_top[-ra] = 0; /* crash if cothread return! */
    co_local_storage_top[-sp] = aligned_sp;
    co_local_storage_top[-fp] = aligned_sp;

    co_local_storage_top[-pc] = (uintptr_t)co_entrypoint;
    co_local_storage_top[-client_entry] = (uintptr_t)entrypoint;

    return co_local_storage_top;
}

void co_switch(cothread_t handle) {
    cothread_t co_previous_handle = co_active_handle; 
    co_swap(co_active_handle = handle, co_previous_handle);
}

#ifdef __cplusplus
}
#endif
