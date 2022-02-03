//Author : Ventery
#ifndef LOG_CURRENTTHREAD_H
#define LOG_CURRENTTHREAD_H

#include <stdint.h>

namespace Log
{
  namespace CurrentThread
  {
    // internal
    extern __thread int t_cachedTid;
    extern __thread char t_tidString[32];
    extern __thread int t_tidStringLength;
    extern __thread const char *t_threadName;
    void cacheTid();
    inline int tid()
    {
      if (__builtin_expect(t_cachedTid == 0, 0))
      {
        cacheTid();
      }
      return t_cachedTid;
    }

    inline const char *tidString() // for logging
    {
      return t_tidString;
    }

    inline int tidStringLength() // for logging
    {
      return t_tidStringLength;
    }

    inline const char *name() { return t_threadName; }
  }
} //namespace Log

#endif // LOG_CURRENTTHREAD_H
