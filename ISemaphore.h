#pragma once

#include <stddef.h>

class ISemaphore
{
public:
    virtual ~ISemaphore() = default;
    virtual void wait() = 0;
    virtual bool tryWait() = 0;
    virtual bool waitTimeout(size_t timeout_ms) = 0;
    virtual void post() = 0;
    virtual bool isValid() const = 0;
};
