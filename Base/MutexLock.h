#ifndef BASE_MUTEXLOCK_H
#define BASE_MUTEXLOCK_H

#include <pthread.h>
#include <cstdio>
#include "noncopyable.h"

namespace Global
{
    class MutexLock : Noncopyable
    {
    public:
        MutexLock() { pthread_mutex_init(&mutex, NULL); }
        ~MutexLock()
        {
            pthread_mutex_lock(&mutex);
            pthread_mutex_destroy(&mutex);
        }
        void lock() { pthread_mutex_lock(&mutex); }
        void unlock() { pthread_mutex_unlock(&mutex); }
        pthread_mutex_t *get() { return &mutex; }

    private:
        pthread_mutex_t mutex;
    };

    class MutexLockGuard : Noncopyable
    {
    public:
        explicit MutexLockGuard(MutexLock &_mutex) : mutex(_mutex) { mutex.lock(); }
        ~MutexLockGuard() { mutex.unlock(); }

    private:
        MutexLock &mutex;
    };
} //namespace Global

#endif //BASE_MUTEXLOCK_H
