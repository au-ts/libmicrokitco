#include <microkit.h>
#include <libmicrokitco.h>
#include <printf.h>

#define MAGIC_1 0xDEADBEEF
#define MAGIC_2 0x8BADF00D
#define MAGIC_3 0xBAD22222

uintptr_t co_mem;
int stack_size = 0x1000;
uintptr_t stack1;
uintptr_t stack2;
uintptr_t stack3;

size_t co_entry1() {
    size_t retval;
    printf("Deadlock-free PD: co1: joining co2\n");
    co_err_t err = microkit_cothread_join(2, &retval);
    if (err != co_no_err) {
        printf("Deadlock-free PD: co1: cannot join, err: %s\n", microkit_cothread_pretty_error(err));
    }

    if (retval != MAGIC_2) {
        printf("Deadlock-free PD: co1: magic 2 NOT correct\n");
    } else {
        printf("Deadlock-free PD: co1: magic 2 correct, returning magic 3\n");
    }

    return MAGIC_3;
};

size_t co_entry2() {
    size_t retval;
    printf("Deadlock-free PD: co2: joining co3\n");
    co_err_t err = microkit_cothread_join(3, &retval);
    if (err != co_no_err) {
        printf("Deadlock-free PD: co1: cannot join, err: %s\n", microkit_cothread_pretty_error(err));
    }

    if (retval != MAGIC_1) {
        printf("Deadlock-free PD: co2: magic 1 NOT correct\n");
    } else {
        printf("Deadlock-free PD: co2: magic 1 correct, return magic 2\n");
    }

    return MAGIC_2;
};

size_t co_entry3() {
    printf("Deadlock-free PD: co3: return magic 1\n");
    return MAGIC_1;
};

void init(void) {
    printf("Deadlock-free PD: starting\n");

    co_err_t err = microkit_cothread_init(co_mem, stack_size, 3, stack1, stack2, stack3);
    if (err != co_no_err) {
        printf("Deadlock-free PD: ERR: cannot init libmicrokitco\n");
        return;
    }
    printf("Deadlock-free PD: libmicrokitco started\n");

    microkit_cothread_t co1, co2, co3;
    
    microkit_cothread_spawn(co_entry1, ready_true, &co1, 0);
    microkit_cothread_spawn(co_entry2, ready_true, &co2, 0);
    microkit_cothread_spawn(co_entry3, ready_true, &co3, 0);

    printf("Deadlock-free PD: cothreads spawned\n");

    size_t retval;
    printf("Deadlock-free PD: root: joining co1\n");
    microkit_cothread_join(1, &retval);
    if (retval == MAGIC_3) {
        printf("Deadlock-free PD: root: magic 3 correct\n");
    } else {
        printf("Deadlock-free PD: root: magic 3 NOT correct\n");
    }
    
}

void notified(microkit_channel channel) {

}
