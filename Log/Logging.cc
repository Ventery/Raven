#include "Logging.h"
#include "CurrentThread.h"
#include "Thread.h"
#include "AsyncLogging.h"
#include <assert.h>
#include <iostream>
#include <time.h>
#include <sys/time.h>

namespace Log
{
    static pthread_once_t once_control_ = PTHREAD_ONCE_INIT;
    static AsyncLogging *AsyncLogger_;

    std::string Logger::logFileName_ = "./WebServer.log";
    int Logger::ClassNum_ = 0;

    void once_init()
    {
        AsyncLogger_ = new AsyncLogging(Logger::getLogFileName());
        AsyncLogger_->start();
    }

    void output(const char *msg, int len)
    {
        pthread_once(&once_control_, once_init);
        AsyncLogger_->append(msg, len);
    }

    Logger::Impl::Impl(const char *fileName, int line)
        : stream_(),
          line_(line),
          basename_(fileName)
    {
        formatTime();
    }

    void Logger::Impl::formatTime()
    {
        struct timeval tv;
        time_t time;
        char str_t[26] = {0};
        gettimeofday(&tv, NULL);
        time = tv.tv_sec;
        struct tm *p_time = localtime(&time);
        strftime(str_t, 26, "%Y-%m-%d %H:%M:%S\n", p_time);
        stream_ << str_t;
        stream_ << __FILE__ << " : " << __LINE__;
        stream_ << "\n+-----------------------+\n";
    }

    Logger::Logger(const char *fileName, int line)
        : impl_(fileName, line)
    {
        //printf("Class Logger num:%d Created!\n",++ClassNum_);
    }

    Logger::~Logger()
    {
        //printf("Class Logger num:%d Deleted!\n",ClassNum_);
        impl_.stream_ << "\n+-----------------------+\n\n";
        //impl_.stream_ << " -- " << impl_.basename_ << ':' << impl_.line_ << '\n';
        //impl_.stream_ << "-------------------------\n";

        const LogStream::Buffer &buf(stream().buffer());
        output(buf.data(), buf.length());
    }
} //namespace Log
