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

        std::shared_ptr<MutexLockGuard> gardPtr_;
        int formerLength_;
    };

    class ProgressBarDemo : public Noncopyable
    {
    public:
        ProgressBarDemo(const std::string &title, const std::string &measure, long amount) : title_(title), measure_(measure), amount_(amount) {}
        std::string getBar(long number)
        {
            std::string result;
            result += "+----------------------------------------+\n";
            result += title_ + " : " + std::to_string(number) + " " + measure_ + " (" + std::to_string(((int)((number * 10000.0) / amount_)) / 100.0) + "%)\n";
            result += "[";
            int sum = (int)((number * 40.0) / amount_);
            for (int i = 1; i <= 40; i++)
            {
                if (i <= sum)
                    result += "|";
                else
                    (result += " ");
            }
            result += "]\n";
            result += "+----------------------------------------+\n";

            return result;
        }

    private:
        std::string title_;
        std::string measure_;
        long amount_;
    };
} // namespace Global

#endif // BASE_SCREENREFRESHER_H