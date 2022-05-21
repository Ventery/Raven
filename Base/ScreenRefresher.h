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
        ScreenRefresher() : formerLength_(0) { gardPtr_ = std::make_shared<MutexLockGuard>(mRefreshScreamer); }

        void rePrint(const std::string &string)
        {
            backSpace(formerLength_);
            std::cout << string;
            std::cout << std::flush;
            formerLength_ = string.length();
        }

    private:
        static void backSpace(int num)
        {
            for (int i = 0; i < num; i++)
            {
                std::cout << "\b";
            }

            for (int i = 0; i < num; i++)
            {
                std::cout << " ";
            }

            for (int i = 0; i < num; i++)
            {
                std::cout << "\b";
            }
        }

        std::shared_ptr<MutexLockGuard> gardPtr_;
        int formerLength_;
    };

    class ProgressBarDemo : public Noncopyable
    {
    public:
        ProgressBarDemo(const std::string &measure, long amount) : measure_(measure), amount_(amount) {}
        std::string getBar(long number)
        {
            static int sBarLength = 60;
            std::string result;
            int percentage = (int)((number * 10000.0) / amount_);
            int first = percentage / 100;
            int second = percentage % 100;
            result += "[";
            int sum = (int)((number * sBarLength * 1.0) / amount_);
            for (int i = 1; i <= sBarLength; i++)
            {
                if (i <= sum)
                    result += "#";
                else
                    (result += " ");
            }
            result += "]" + std::to_string(number) + " " + measure_ + " (" + std::to_string(first) + "." + std::to_string(second) + "%)";

            return result;
        }

    private:
        std::string measure_;
        long amount_;
    };
} // namespace Global

#endif // BASE_SCREENREFRESHER_H