// role : Output some refreshable information.
#ifndef BASE_SCREENREFRESHER_H
#define BASE_SCREENREFRESHER_H

#include <memory>

#include "Global.h"
#include "MutexLock.h"
#include "noncopyable.h"

namespace Global
{
    class ScreenRefresher : public Noncopyable
    {
    public:
        ScreenRefresher() { gardPtr = std::make_shared<MutexLockGuard>(mRefreshScreamer); }

        ScreenRefresher &operator>>(const std::string &string)
        {
            backSpace(formerLength);
            std::cout << string << std::fflush;
            formerLength = string.length();
        }

    private:
        static void backSpace(int num)
        {
            for (int i = 0; i < num; i++)
            {
                std::cout << '\b';
            }

            for (int i = 0; i < num; i++)
            {
                std::cout << ' ';
            }

            for (int i = 0; i < num; i++)
            {
                std::cout << '\b';
            }
        }

        std::shared_ptr<MutexLockGuard> gardPtr;
        int formerLength;
    };
} // namespace Global

#endif // BASE_SCREENREFRESHER_H