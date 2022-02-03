//role : Singleton base class.
//Author : Ventery
#ifndef BASE_SINGLETON_H
#define BASE_SINGLETON_H

#include <assert.h>
#include <pthread.h>

#include "noncopyable.h"

namespace Global
{
    template <typename T>
    class Singleton : Noncopyable
    {
    public:
        Singleton(){};
        ~Singleton(){};
        static const T &GetInstance()
        {
            pthread_once(&ponce_, &Singleton::init);
            assert(instance_ != NULL);
            return *instance_;
        }

    private:
        static void init()
        {
            instance_ = new T();
        }

        static pthread_once_t ponce_;
        static const T *instance_;
    };

    template <typename T>
    const T *Singleton<T>::instance_ = nullptr;

    template<typename T>
    pthread_once_t Singleton<T>::ponce_ = PTHREAD_ONCE_INIT;

} // namespace Global

#endif