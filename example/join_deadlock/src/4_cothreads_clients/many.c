#include <microkit.h>
#include <libmicrokitco.h>
#include <printf.h>

uintptr_t co_mem;
int stack_size = 0x1000;
uintptr_t stack1;
uintptr_t stack2;
uintptr_t stack3;
uintptr_t stack4;

size_t cothreads_return_codes[5] = { 0 };

#define MAGIC 0x42

// entrypoints #1, #2 and #3 will join #4
void co_entry1() {
    printf("Many to one PD: co1: joining co4\n");
    co_err_t err = microkit_cothread_join(4);
    if (err != co_no_err) {
        printf("Many to one PD: co1: cannot join, err: %s\n", microkit_cothread_pretty_error(err));
    } else if (cothreads_return_codes[4] != MAGIC) {
        printf("Many to one PD: co1: wrong magic returned %p\n", cothreads_return_codes[4]);
    } else {
        printf("Many to one PD: co1: awaken with correct magic\n");
    }
    cothreads_return_codes[1] = cothreads_return_codes[4] >> 4;
};

void co_entry2() {
    printf("Many to one PD: co2: joining co4\n");
    co_err_t err = microkit_cothread_join(4);
    if (err != co_no_err) {
        printf("Many to one PD: co2: cannot join, err: %s\n", microkit_cothread_pretty_error(err));
    } else if (cothreads_return_codes[4] != MAGIC) {
        printf("Many to one PD: co2: wrong magic returned %p\n", cothreads_return_codes[4]);
    } else {
        printf("Many to one PD: co2: awaken with correct magic\n");
    }
    cothreads_return_codes[2] = cothreads_return_codes[4] >> 8;
};

void co_entry3() {
    printf("Many to one PD: co3: joining co4\n");
    co_err_t err = microkit_cothread_join(4);
    if (err != co_no_err) {
        printf("Many to one PD: co3: cannot join, err: %s\n", microkit_cothread_pretty_error(err));
    } else if (cothreads_return_codes[4] != MAGIC) {
        printf("Many to one PD: co3: wrong magic returned %p\n", cothreads_return_codes[4]);
    } else {
        printf("Many to one PD: co3: awaken with correct magic\n");
    }
    cothreads_return_codes[3] = cothreads_return_codes[4] >> 12;
};

void co_entry4() {
    printf("Many to one PD: co4 returning magic\n");
    cothreads_return_codes[4] = MAGIC;
};

void init(void) {
    printf("Many to one PD: starting\n");

    co_err_t err = microkit_cothread_init(
        co_mem, 
        stack_size, 
        stack1, 
        stack2, 
        stack3,
        stack4
    );
    if (err != co_no_err) {
        printf("Many to one PD: ERR: cannot init libmicrokitco\n");
        return;
    }
    printf("Many to one PD: libmicrokitco started\n");

    microkit_cothread_t co1, co2, co3, co4;
    
    microkit_cothread_spawn(co_entry1, true, &co1, 0);
    microkit_cothread_spawn(co_entry2, true, &co2, 0);
    microkit_cothread_spawn(co_entry3, true, &co3, 0);
    microkit_cothread_spawn(co_entry4, true, &co4, 0);

    printf("Many to one PD: cothreads %d %d %d %d spawned\n", co1, co2, co3, co4);

    printf("Many to one PD: root: joining co1\n");
    co_err_t join_err1 = microkit_cothread_join(1);
    printf("Many to one PD: root: joining co2\n");
    co_err_t join_err2 = microkit_cothread_join(2);
    printf("Many to one PD: root: joining co3\n");
    co_err_t join_err3 = microkit_cothread_join(3);

    size_t retval1 = cothreads_return_codes[1];
    size_t retval2 = cothreads_return_codes[2];
    size_t retval3 = cothreads_return_codes[3];

    if (join_err1 != co_no_err || join_err2 != co_no_err || join_err3 != co_no_err) {
        printf("Many to one PD: root: cannot do one or more joins, errs are %d %d %d\n", join_err1, join_err2, join_err3);
        microkit_internal_crash(0);
    }

    if (retval1 != MAGIC >> 4 || retval2 != MAGIC >> 8 || retval3 != MAGIC >> 12) {
        printf("Many to one PD: root: one or more retval does not match magic %p %p %p\n", retval1, retval2, retval3);
        microkit_internal_crash(0);
    }

    // joining again after reading the retval should yield err
    if (microkit_cothread_join(3) != co_err_generic_not_initialised) {
        printf("Many to one PD: root: was able to re-read retval\n");
        microkit_internal_crash(0);
    }

    printf("Many to one PD: root: all magic CORRECT, finished OK\n");
}

void notified(microkit_channel channel) {

}
