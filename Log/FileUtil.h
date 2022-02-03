//Author : Ventery

#ifndef LOG_FILEUNTL_H
#define LOG_FILEUNTL_H

#include <string>
#include "../Base/noncopyable.h"

namespace Log
{
  class AppendFile : Noncopyable
  {
  public:
    explicit AppendFile(std::string filename);
    ~AppendFile();
    // append 会向文件写
    void append(const char *logline, const size_t len);
    void flush();

  private:
    size_t write(const char *logline, size_t len);
    FILE *fp_;
    char buffer_[64 * 1024];
  };
} //namespace Log

#endif // LOG_FILEUNTL_H