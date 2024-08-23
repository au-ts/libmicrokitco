#ifndef LIBCO_H
#define LIBCO_H

#ifdef __cplusplus
extern "C" {
#endif

// cothread handle
typedef void* cothread_t;

void co_initialize(void);
cothread_t co_active(void);
cothread_t co_derive(void*, unsigned int, void (*)(void));
void co_switch(cothread_t);

#ifdef __cplusplus
}
#endif

/* ifndef LIBCO_H */
#endif
