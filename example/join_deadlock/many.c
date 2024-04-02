#include <microkit.h>
#include <libmicrokitco.h>
#include <printf.h>

uintptr_t co_mem;
int stack_size = 0x1000;
uintptr_t stack1;
uintptr_t stack2;
uintptr_t stack3;
uintptr_t stack4;

#define MAGIC 0x42

// entrypoints #1, #2 and #3 will join #4
size_t co_entry1() {
    size_t retval;
    printf("Many to one PD: co1: joining co4\n");
    co_err_t err = microkit_cothread_join(4, &retval);
    if (err != co_no_err) {
        printf("Many to one PD: co1: cannot join, err: %s\n", microkit_cothread_pretty_error(err));
    } else if (retval != MAGIC) {
        printf("Many to one PD: co1: wrong magic returned %p\n", retval);
    } else {
        printf("Many to one PD: co1: awaken with correct magic\n");
    }
    return retval;
};

size_t co_entry2() {
    size_t retval;
    printf("Many to one PD: co2: joining co4\n");
    co_err_t err = microkit_cothread_join(4, &retval);
    if (err != co_no_err) {
        printf("Many to one PD: co2: cannot join, err: %s\n", microkit_cothread_pretty_error(err));
    } else if (retval != MAGIC) {
        printf("Many to one PD: co2: wrong magic returned %p\n", retval);
    } else {
        printf("Many to one PD: co2: awaken with correct magic\n");
    }
    return retval;
};

size_t co_entry3() {
    size_t retval;
    printf("Many to one PD: co3: joining co4\n");
    co_err_t err = microkit_cothread_join(4, &retval);
    if (err != co_no_err) {
        printf("Many to one PD: co3: cannot join, err: %s\n", microkit_cothread_pretty_error(err));
    } else if (retval != MAGIC) {
        printf("Many to one PD: co3: wrong magic returned %p\n", retval);
    } else {
        printf("Many to one PD: co3: awaken with correct magic\n");
    }
    return retval;
};

size_t co_entry4() {
    printf("Many to one PD: co4 returning magic\n");
    return MAGIC;
};

void init(void) {
    printf("Many to one PD: starting\n");

    co_err_t err = microkit_cothread_init(co_mem, stack_size, 4, stack1, stack2, stack3, stack4);
    if (err != co_no_err) {
        printf("Many to one PD: ERR: cannot init libmicrokitco\n");
        return;
    }
    printf("Many to one PD: libmicrokitco started\n");

    microkit_cothread_t co1, co2, co3, co4;
    
    microkit_cothread_spawn(co_entry1, ready_true, &co1, 0);
    microkit_cothread_spawn(co_entry2, ready_true, &co2, 0);
    microkit_cothread_spawn(co_entry3, ready_true, &co3, 0);
    microkit_cothread_spawn(co_entry4, ready_true, &co4, 0);

    printf("Many to one PD: cothreads %d %d %d %d spawned\n", co1, co2, co3, co4);

    microkit_cothread_yield();
    microkit_cothread_yield();
    microkit_cothread_yield();

    // size_t retval1, retval2, retval3;
    // printf("Many to one PD: root: joining co1\n");
    // co_err_t join_err1 = microkit_cothread_join(1, &retval1);
    // co_err_t join_err2 = microkit_cothread_join(2, &retval2);
    // co_err_t join_err3 = microkit_cothread_join(3, &retval3);
    // if (join_err1 != co_no_err || join_err2 != co_no_err || join_err3 != co_no_err) {
    //     printf("Many to one PD: root: cannot do one or more joins, errs are %d %d %d\n", join_err1, join_err2, join_err3);
    //     microkit_internal_crash(0);
    // }

    // if (retval1 != MAGIC || retval2 != MAGIC || retval3 != MAGIC) {
    //     printf("Many to one PD: root: one or more retval does not match magic %p %p %p\n", retval1, retval2, retval3);
    //     microkit_internal_crash(0);
    // }
}

void notified(microkit_channel channel) {

}
