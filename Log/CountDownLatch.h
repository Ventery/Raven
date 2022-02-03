//Author : Ventery

#ifndef LOG_COUNTDOWNLATCH_H
#define LOG_COUNTDOWNLATCH_H

#include "Condition.h"
#include "MutexLock.h"
#include "../Base/noncopyable.h"

// CountDownLatch的主要作用是确保Thread中传进去的func真的启动了以后
// 外层的start才返回
namespace Log
{
  class CountDownLatch : Noncopyable
  {
  public:
    explicit CountDownLatch(int count);
    void wait();
    void countDown();

  private:
    mutable MutexLock mutex_;
    Condition condition_;
    int count_;
  };
} //namespace Log

#endif // LOG_COUNTDOWNLATCH_H