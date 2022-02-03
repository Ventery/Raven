//Author : Ventery

#ifndef LOG_LOGFILE_H
#define LOG_LOGFILE_H

#include <memory>
#include <string>
#include "FileUtil.h"
#include "MutexLock.h"
#include "../Base/noncopyable.h"

// TODO 提供自动归档功能
namespace Log
{
  class LogFile : Noncopyable
  {
  public:
    // 每被append flushEveryN次，flush一下，会往文件写，只不过，文件也是带缓冲区的
    LogFile(const std::string &basename, int flushEveryN = 1024);
    ~LogFile();

    void append(const char *logline, int len);
    void flush();
    bool rollFile();

  private:
    void append_unlocked(const char *logline, int len);

    const std::string basename_;
    const int flushEveryN_;

    int count_;
    std::unique_ptr<MutexLock> mutex_;
    std::unique_ptr<AppendFile> file_;
  };
} //namespace Log

#endif // LOG_LOGFILE_H