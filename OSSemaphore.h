#pragma once

#include "ISemaphore.h"
#include <string>

#ifndef _WIN32
#include <semaphore.h>
#include <fcntl.h>
using SEM_HANDLE = sem_t*;
#else
#include <windows.h>
using SEM_HANDLE = HANDLE;
#endif

class OSSemaphore : public ISemaphore
{
public:
    OSSemaphore(const std::string &name, bool create, int initialValue);
    ~OSSemaphore() override;

    void wait() override;
    bool tryWait() override;
    bool waitTimeout(size_t timeout_ms) override;
    void post() override;
    bool isValid() const override;

    static void unlink(const std::string &name);

private:
    SEM_HANDLE m_sem;
    std::string m_name;
};
