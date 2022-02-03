//Author : Ventery

#ifndef LOG_LOGGING_H
#define LOG_LOGGING_H

#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include "LogStream.h"

namespace Log
{
  class AsyncLogging;

  class Logger
  {
  public:
    Logger(const char *fileName, int line);
    ~Logger();
    LogStream &stream() { return impl_.stream_; }

    static void setLogFileName(std::string fileName) { logFileName_ = fileName; }
    static std::string getLogFileName() { return logFileName_; }

  private:
    class Impl
    {
    public:
      Impl(const char *fileName, int line);
      void formatTime();

      LogStream stream_;
      int line_;
      std::string basename_;
    };
    Impl impl_;
    static std::string logFileName_;
    static int ClassNum_;
  };

#define LOG Logger(__FILE__, __LINE__).stream()
} //namespace Log

#endif // LOG_LOGGING_H