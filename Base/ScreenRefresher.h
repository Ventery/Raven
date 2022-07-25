// role : Output some refreshable information.
#ifndef SCREENREFRESHER_H
#define SCREENREFRESHER_H

#include <memory>
#include <stdio.h>

#include "MutexLock.h"
#include "noncopyable.h"

class ProgressBarDemo : public Noncopyable
{
public:
    ProgressBarDemo(const std::string &measure, long amount) : measure_(measure), amount_(amount) ,isCompleted(false){}

    bool IsCompleted() {return isCompleted;}
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
        if (first == 100)
        {
            isCompleted = true;
            result+="\n";
        }
        return result;
    }

private:
    std::string measure_;
    long amount_;
    bool isCompleted;
};

class ScreenRefresher : public Noncopyable
{
public:
    ScreenRefresher(const std::string &measure, long amount) : formerLength_(0) ,
             gardPtr_ (std::make_shared<MutexLockGuard>(mRefreshScreamer_)),
             progressBar_(measure,amount)
    {
    }

    ~ScreenRefresher()
    {
        if (!progressBar_.IsCompleted())
        {
            std::cout<<std::endl<<"Progress is not completed!"<<std::endl;
        }
    }

    void rePrint(long newValue)
    {
        std::string newBar =  progressBar_.getBar(newValue);
        backSpace(formerLength_);
        std::cout << newBar;
        std::cout << std::flush;
        formerLength_ = newBar.length();
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

        std::cout << std::flush;
    }

    int formerLength_;
    std::shared_ptr<MutexLockGuard> gardPtr_;
    ProgressBarDemo progressBar_;

    static MutexLock mRefreshScreamer_;
};
MutexLock ScreenRefresher::mRefreshScreamer_;

#endif // SCREENREFRESHER_H