#include <microkit.h>
#include <libmicrokitco.h>
#include <printf.h>

uintptr_t co_mem;
int stack_size = 0x1000;
uintptr_t stack1;
uintptr_t stack2;
uintptr_t stack3;

#define MAGIC 0x42

size_t co_entry1() {
    size_t retval;
    printf("Deadlocked PD: co1: joining co2\n");
    co_err_t err = microkit_cothread_join(2, &retval);
    if (err != co_no_err) {
        printf("Deadlocked PD: co1: cannot join, err: %s\n", microkit_cothread_pretty_error(err));
    }
    return retval;
};

size_t co_entry2() {
    size_t retval;
    printf("Deadlocked PD: co2: joining co3\n");
    co_err_t err = microkit_cothread_join(3, &retval);
    if (err != co_no_err) {
        printf("Deadlocked PD: co1: cannot join, err: %s\n", microkit_cothread_pretty_error(err));
    }
    return retval;
};

size_t co_entry3() {
    size_t _retval;
    printf("Deadlocked PD: co3: joining co1\n");
    // 1 -> 2 -> 3 \, deadlocked!
    // ^-----------|
    co_err_t err = microkit_cothread_join(1, &_retval);
    if (err == co_err_join_deadlock_detected) {
        printf("Deadlocked PD: co3: DEADLOCK DETECTED...OK\n");
    } else {
        printf("Deadlocked PD: co3: ERROR: DEADLOCK NOT DETECTED\n");
        microkit_internal_crash(err);
    }
    return MAGIC;
};

void init(void) {
    printf("Deadlocked PD: starting\n");

    co_err_t err = microkit_cothread_init(
        co_mem, 
        stack_size,  
        stack1, 
        stack2, 
        stack3
    );
    if (err != co_no_err) {
        printf("Deadlocked PD: ERR: cannot init libmicrokitco\n");
        return;
    }
    printf("Deadlocked PD: libmicrokitco started\n");

    microkit_cothread_t co1, co2, co3;
    
    microkit_cothread_spawn(co_entry1, true, &co1, 0);
    microkit_cothread_spawn(co_entry2, true, &co2, 0);
    microkit_cothread_spawn(co_entry3, true, &co3, 0);

    printf("Deadlocked PD: cothreads spawned\n");

    size_t retval;
    printf("Deadlocked PD: root: joining co1\n");
    err = microkit_cothread_join(1, &retval);
    if (err != co_no_err) {
        printf("Deadlocked PD: root: cannot join, err is %s\n", microkit_cothread_pretty_error(err));
    } else if (retval == MAGIC){
        printf("Deadlocked PD: root: finished ok\n");
    } else {
        printf("Deadlocked PD: root: magic does not match %p\n", retval);
    }
}

void notified(microkit_channel channel) {

}
