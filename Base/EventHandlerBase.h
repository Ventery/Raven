#ifndef BASE_EVENTHANDLERBSDE_H
#define BASE_EVENTHANDLERBSDE_H

#include <functional>

#include "noncopyable.h"

namespace Global
{
    class EventHandlerBase : public Noncopyable
    {
    public:
        typedef std::function<void()> CallBack;

    protected:
        virtual void signalHandler(int sig)
        {
            throw "You should not use this function";
        };
    };
} //namespace Global

#endif //BASE_EVENTHANDLERBSDE_H