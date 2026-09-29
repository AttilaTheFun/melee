#ifndef MELEE_COND_H
#define MELEE_COND_H
#include <pthread.h>
#include <time.h>
#include <stdlib.h>
/* Non-Apple callers initialize the condition with CLOCK_MONOTONIC before any
 * workers start. Relative deadlines must not change when wall time changes. */
static inline void melee_cond_init_monotonic(pthread_cond_t* condition)
{
#ifndef __APPLE__
    pthread_condattr_t attributes;
    if (pthread_condattr_init(&attributes) ||
        pthread_condattr_setclock(&attributes, CLOCK_MONOTONIC) ||
        pthread_cond_init(condition, &attributes)) abort();
    pthread_condattr_destroy(&attributes);
#else
    (void)condition;
#endif
}
static inline int melee_cond_wait_relative(pthread_cond_t* condition,
                                          pthread_mutex_t* mutex,
                                          const struct timespec* relative)
{
#ifdef __APPLE__
    return pthread_cond_timedwait_relative_np(condition, mutex, relative);
#else
    struct timespec deadline;
    clock_gettime(CLOCK_MONOTONIC, &deadline);
    deadline.tv_sec += relative->tv_sec;
    deadline.tv_nsec += relative->tv_nsec;
    if (deadline.tv_nsec >= 1000000000) {
        ++deadline.tv_sec; deadline.tv_nsec -= 1000000000;
    }
    return pthread_cond_timedwait(condition, mutex, &deadline);
#endif
}
#endif
