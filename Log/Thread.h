//Author : Ventery

#ifndef LOG_THREAD_H
#define LOG_THREAD_H

#include <pthread.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <functional>
#include <memory>
#include <string>
#include "CountDownLatch.h"
#include "../Base/noncopyable.h"

namespace Log
{
  class Thread : Noncopyable
  {
  public:
    typedef std::function<void()> ThreadFunc;
    explicit Thread(const ThreadFunc &, const std::string &name = std::string());
    ~Thread();
    void start();
    int join();
    bool started() const { return started_; }
    pid_t tid() const { return tid_; }
    const std::string &name() const { return name_; }

  private:
    void setDefaultName();
    bool started_;
    bool joined_;
    pthread_t pthreadId_;
    pid_t tid_;
    ThreadFunc func_;
    std::string name_;
    CountDownLatch latch_;
  };
} //namespace Log

#endif // LOG_THREAD_H