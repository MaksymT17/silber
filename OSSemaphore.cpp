#include "OSSemaphore.h"
#include "SilberLogging.h"
#include <chrono>
#include <thread>

#ifndef _WIN32
#include <unistd.h>
#include <time.h>
#include <cstring>
#include <cerrno>

OSSemaphore::OSSemaphore(const std::string &name, bool create, int initialValue)
    : m_name(name)
{
    // Mode parameter not used if not creating, but O_CREAT demands 4 arguments
    m_sem = sem_open(m_name.c_str(), O_CREAT, 0666, initialValue);
    if (m_sem == SEM_FAILED)
    {
        reportSilberError("OSSemaphore sem_open failure for: %s error: %s", m_name.c_str(), strerror(errno));
    }
}

OSSemaphore::~OSSemaphore()
{
    if (m_sem != SEM_FAILED)
    {
        if (sem_close(m_sem) == -1)
        {
            reportSilberError("Failed to close semaphore %s: %s", m_name.c_str(), strerror(errno));
        }
    }
}

void OSSemaphore::wait()
{
    if (m_sem != SEM_FAILED)
    {
        sem_wait(m_sem);
    }
}

bool OSSemaphore::tryWait()
{
    if (m_sem == SEM_FAILED) return false;
    return sem_trywait(m_sem) == 0;
}

bool OSSemaphore::waitTimeout(size_t timeout_ms)
{
    if (m_sem == SEM_FAILED) return false;

#ifdef __APPLE__
    // macOS fallback: polling loop
    auto start = std::chrono::steady_clock::now();
    while (true)
    {
        if (sem_trywait(m_sem) == 0)
        {
            return true;
        }
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
        if (elapsed >= (long long)timeout_ms)
        {
            break;
        }
        size_t sleep_time = 5;
        if (elapsed + sleep_time > timeout_ms)
        {
            sleep_time = timeout_ms - elapsed;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
    }
    return false;
#else
    // Linux: native timed wait
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
    {
        return false;
    }
    ts.tv_sec += timeout_ms / 1000;
    ts.tv_nsec += (timeout_ms % 1000) * 1000000;
    if (ts.tv_nsec >= 1000000000)
    {
        ts.tv_sec += 1;
        ts.tv_nsec -= 1000000000;
    }
    return sem_timedwait(m_sem, &ts) == 0;
#endif
}

void OSSemaphore::post()
{
    if (m_sem != SEM_FAILED)
    {
        sem_post(m_sem);
    }
}

bool OSSemaphore::isValid() const
{
    return m_sem != SEM_FAILED;
}

void OSSemaphore::unlink(const std::string &name)
{
    sem_unlink(name.c_str());
}

#else

OSSemaphore::OSSemaphore(const std::string &name, bool create, int initialValue)
    : m_name(name)
{
    std::wstring wname(m_name.begin(), m_name.end());
    m_sem = CreateSemaphoreW(NULL, initialValue, MAXLONG, wname.c_str());
    if (m_sem == NULL)
    {
        reportSilberError("OSSemaphore CreateSemaphoreW failure for: %s error: %d", m_name.c_str(), GetLastError());
    }
}

OSSemaphore::~OSSemaphore()
{
    if (m_sem != NULL)
    {
        CloseHandle(m_sem);
    }
}

void OSSemaphore::wait()
{
    if (m_sem != NULL)
    {
        WaitForSingleObject(m_sem, INFINITE);
    }
}

bool OSSemaphore::tryWait()
{
    if (m_sem == NULL) return false;
    return WaitForSingleObject(m_sem, 0) == WAIT_OBJECT_0;
}

bool OSSemaphore::waitTimeout(size_t timeout_ms)
{
    if (m_sem == NULL) return false;
    return WaitForSingleObject(m_sem, (DWORD)timeout_ms) == WAIT_OBJECT_0;
}

void OSSemaphore::post()
{
    if (m_sem != NULL)
    {
        ReleaseSemaphore(m_sem, 1, NULL);
    }
}

bool OSSemaphore::isValid() const
{
    return m_sem != NULL;
}

void OSSemaphore::unlink(const std::string &name)
{
    // No-op on Windows
}

#endif
