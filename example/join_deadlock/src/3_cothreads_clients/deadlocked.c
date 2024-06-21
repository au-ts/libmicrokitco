#include <microkit.h>
#include <libmicrokitco.h>
#include <printf.h>

uintptr_t co_mem;
int stack_size = 0x1000;
uintptr_t stack1;
uintptr_t stack2;
uintptr_t stack3;

size_t cothreads_return_codes[4] = { 0 };

#define MAGIC 0x42

void co_entry1() {
    printf("Deadlocked PD: co1: joining co2\n");
    co_err_t err = microkit_cothread_join(2);
    if (err != co_no_err) {
        printf("Deadlocked PD: co1: cannot join, err: %s\n", microkit_cothread_pretty_error(err));
    }
    cothreads_return_codes[1] = cothreads_return_codes[2];
};

void co_entry2() {
    printf("Deadlocked PD: co2: joining co3\n");
    co_err_t err = microkit_cothread_join(3);
    if (err != co_no_err) {
        printf("Deadlocked PD: co1: cannot join, err: %s\n", microkit_cothread_pretty_error(err));
    }
    cothreads_return_codes[2] = cothreads_return_codes[3];
};

void co_entry3() {
    printf("Deadlocked PD: co3: joining co1\n");
    // 1 -> 2 -> 3 \, deadlocked!
    // ^-----------|
    co_err_t err = microkit_cothread_join(1);
    if (err == co_err_join_deadlock_detected) {
        printf("Deadlocked PD: co3: DEADLOCK DETECTED...OK\n");
    } else {
        printf("Deadlocked PD: co3: ERROR: DEADLOCK NOT DETECTED\n");
        microkit_internal_crash(err);
    }
    cothreads_return_codes[3] = MAGIC;
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

    printf("Deadlocked PD: root: joining co1\n");
    err = microkit_cothread_join(1);
    if (err != co_no_err) {
        printf("Deadlocked PD: root: cannot join, err is %s\n", microkit_cothread_pretty_error(err));
    } else if (cothreads_return_codes[1] == MAGIC){
        printf("Deadlocked PD: root: finished ok\n");
    } else {
        printf("Deadlocked PD: root: magic does not match %p\n", cothreads_return_codes[1]);
    }
}

void notified(microkit_channel channel) {

}
