#pragma once

#include "ServerProcCommunicator.h"
#include <thread>
#include <chrono>

static constexpr size_t WAIT_POLL_PERIOD = 5;

class ClientProcCommunicator : public ProcCommunicator
{
public:
    ClientProcCommunicator(const std::string &shMemName);
    virtual ~ClientProcCommunicator();

    bool isValid() const;

#ifndef _WIN32
    // Helper for native timed wait on POSIX
    bool sem_wait_timeout(N_SEM sem, size_t timeout_ms)
    {
#ifdef __APPLE__
        // macOS fallback: polling loop
        auto start = std::chrono::steady_clock::now();
        while (true)
        {
            if (sem_trywait(sem) == 0)
            {
                return true;
            }
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
            if (elapsed >= (long long)timeout_ms)
            {
                break;
            }
            size_t sleep_time = WAIT_POLL_PERIOD;
            if (elapsed + sleep_time > timeout_ms)
            {
                sleep_time = timeout_ms - elapsed;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
        }
        return false;
#else
        // Linux: native low-power timed wait
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
        return sem_timedwait(sem, &ts) == 0;
#endif
    }

    void sem_wait_adaptive(N_SEM sem)
    {
        for (int spin = 0; spin < 4000; ++spin)
        {
            if (sem_trywait(sem) == 0)
            {
                return;
            }
            cpu_yield();
        }
        sem_wait(sem);
    }

    bool sem_wait_timeout_adaptive(N_SEM sem, size_t timeout_ms)
    {
        for (int spin = 0; spin < 2000; ++spin)
        {
            if (sem_trywait(sem) == 0)
            {
                return true;
            }
            cpu_yield();
        }
        return sem_wait_timeout(sem, timeout_ms);
    }

    template <typename Response>
    bool sendRequestGetResponse(const Message *request, const Response **reponse)
    {
        if (m_slot_index == -1)
        {
            return false;
        }

        sem_wait_adaptive(m_slave_ready);
        
        // Drain any stale response signals
        while (sem_trywait(m_slave_sent) == 0) {}

        // Set active slot in the control registry
        ClientSlotRegistry *registry = static_cast<ClientSlotRegistry*>(m_sender->getPtr());
        registry->active_slot.store(m_slot_index);

        m_sender->sendMessage(request, CONTROL_PAGE_SIZE + m_slot_index * CLIENT_MEM_SIZE);
        sem_post(m_master_sent);
        sem_wait_adaptive(m_slave_sent);

        const Response *repsonsePtr = static_cast<const Response *>(m_receiver->receiveMessage(CONTROL_PAGE_SIZE + m_slot_index * CLIENT_MEM_SIZE));

        if (repsonsePtr)
        {
            *reponse = repsonsePtr;
            sem_post(m_slave_ready);
            return true;
        }
        
        sem_post(m_slave_ready);
        return false;
    }

    template <typename Response>
    bool sendRequestGetResponse(const Message *request, const Response **reponse, size_t timeout_ms)
    {
        if (m_slot_index == -1)
        {
            return false;
        }

        auto start = std::chrono::steady_clock::now();
        if (!sem_wait_timeout_adaptive(m_slave_ready, timeout_ms))
        {
            return false;
        }

        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
        if (elapsed >= (long long)timeout_ms)
        {
            sem_post(m_slave_ready);
            return false;
        }
        size_t remaining = timeout_ms - elapsed;

        // Drain any stale response signals
        while (sem_trywait(m_slave_sent) == 0) {}

        // Set active slot in the control registry
        ClientSlotRegistry *registry = static_cast<ClientSlotRegistry*>(m_sender->getPtr());
        registry->active_slot.store(m_slot_index);

        m_sender->sendMessage(request, CONTROL_PAGE_SIZE + m_slot_index * CLIENT_MEM_SIZE);
        sem_post(m_master_sent);

        if (!sem_wait_timeout_adaptive(m_slave_sent, remaining))
        {
            sem_post(m_slave_ready);
            return false;
        }

        const Response *repsonsePtr = static_cast<const Response *>(m_receiver->receiveMessage(CONTROL_PAGE_SIZE + m_slot_index * CLIENT_MEM_SIZE));

        if (repsonsePtr)
        {
            *reponse = repsonsePtr;
            sem_post(m_slave_ready);
            return true;
        }
        
        sem_post(m_slave_ready);
        return false;
    }

#else
    void sem_wait_adaptive(N_SEM sem)
    {
        for (int spin = 0; spin < 4000; ++spin)
        {
            if (WaitForSingleObject(sem, 0) == WAIT_OBJECT_0)
            {
                return;
            }
            cpu_yield();
        }
        WaitForSingleObject(sem, INFINITE);
    }

    bool sem_wait_timeout_adaptive(N_SEM sem, size_t timeout_ms)
    {
        for (int spin = 0; spin < 2000; ++spin)
        {
            if (WaitForSingleObject(sem, 0) == WAIT_OBJECT_0)
            {
                return true;
            }
            cpu_yield();
        }
        return WaitForSingleObject(sem, (DWORD)timeout_ms) == WAIT_OBJECT_0;
    }

    template <typename Response>
    bool sendRequestGetResponse(const Message *request, const Response **reponse)
    {
        if (m_slot_index == -1)
        {
            return false;
        }

        sem_wait_adaptive(m_slave_ready);

        // Drain stale signals
        while (WaitForSingleObject(m_slave_sent, 0) == WAIT_OBJECT_0) {}

        // Set active slot in the control registry
        ClientSlotRegistry *registry = static_cast<ClientSlotRegistry*>(m_sender->getPtr());
        registry->active_slot.store(m_slot_index);

        m_sender->sendMessage(request, CONTROL_PAGE_SIZE + m_slot_index * CLIENT_MEM_SIZE);
        ReleaseSemaphore(m_master_sent, 1, NULL);
        sem_wait_adaptive(m_slave_sent);

        const Response *repsonsePtr = static_cast<const Response *>(m_receiver->receiveMessage(CONTROL_PAGE_SIZE + m_slot_index * CLIENT_MEM_SIZE));

        if (repsonsePtr)
        {
            *reponse = repsonsePtr;
            ReleaseSemaphore(m_slave_ready, 1, NULL);
            return true;
        }
        ReleaseSemaphore(m_slave_ready, 1, NULL);
        return false;
    }

    template <typename Response>
    bool sendRequestGetResponse(const Message *request, const Response **reponse, size_t timeout_ms)
    {
        if (m_slot_index == -1)
        {
            return false;
        }

        auto start = std::chrono::steady_clock::now();
        if (!sem_wait_timeout_adaptive(m_slave_ready, timeout_ms))
        {
            return false;
        }

        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
        if (elapsed >= (long long)timeout_ms)
        {
            ReleaseSemaphore(m_slave_ready, 1, NULL);
            return false;
        }
        size_t remaining = timeout_ms - elapsed;

        // Drain stale signals
        while (WaitForSingleObject(m_slave_sent, 0) == WAIT_OBJECT_0) {}

        // Set active slot in the control registry
        ClientSlotRegistry *registry = static_cast<ClientSlotRegistry*>(m_sender->getPtr());
        registry->active_slot.store(m_slot_index);

        m_sender->sendMessage(request, CONTROL_PAGE_SIZE + m_slot_index * CLIENT_MEM_SIZE);
        ReleaseSemaphore(m_master_sent, 1, NULL);

        if (!sem_wait_timeout_adaptive(m_slave_sent, remaining))
        {
            ReleaseSemaphore(m_slave_ready, 1, NULL);
            return false;
        }

        const Response *repsonsePtr = static_cast<const Response *>(m_receiver->receiveMessage(CONTROL_PAGE_SIZE + m_slot_index * CLIENT_MEM_SIZE));

        if (repsonsePtr)
        {
            *reponse = repsonsePtr;
            ReleaseSemaphore(m_slave_ready, 1, NULL);
            return true;
        }
        ReleaseSemaphore(m_slave_ready, 1, NULL);
        return false;
    }
#endif

private:
    int m_slot_index{-1};
};
