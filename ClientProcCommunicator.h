#pragma once

#include "ServerProcCommunicator.h"
#include <thread>
#include <chrono>

static constexpr size_t WAIT_POLL_PERIOD = 5;

class ClientProcCommunicator : public ProcCommunicator
{
public:
    ClientProcCommunicator(const std::string &shMemName);
    ClientProcCommunicator(const std::string &shMemName,
                           std::unique_ptr<ISharedMemorySender> sender,
                           std::unique_ptr<ISharedMemoryReceiver> receiver);
    virtual ~ClientProcCommunicator();

    bool isValid() const;

    void sem_wait_adaptive(ISemaphore *sem)
    {
        for (int spin = 0; spin < 4000; ++spin)
        {
            if (sem->tryWait())
            {
                return;
            }
            cpu_yield();
        }
        sem->wait();
    }

    bool sem_wait_timeout_adaptive(ISemaphore *sem, size_t timeout_ms)
    {
        for (int spin = 0; spin < 2000; ++spin)
        {
            if (sem->tryWait())
            {
                return true;
            }
            cpu_yield();
        }
        return sem->waitTimeout(timeout_ms);
    }

    template <typename Response>
    bool sendRequestGetResponse(const Message *request, const Response **response)
    {
        if (!isValid())
        {
            return false;
        }

        sem_wait_adaptive(m_slave_ready.get());
        
        // Drain any stale response signals
        while (m_slave_sent->tryWait()) {}

        // Set active slot in the control registry
        ClientSlotRegistry *registry = static_cast<ClientSlotRegistry*>(m_sender->getPtr());
        registry->active_slot.store(m_slot_index);

        m_sender->sendMessage(request, CONTROL_PAGE_SIZE + m_slot_index * CLIENT_MEM_SIZE);
        m_master_sent->post();
        sem_wait_adaptive(m_slave_sent.get());

        const Response *repsonsePtr = static_cast<const Response *>(m_receiver->receiveMessage(CONTROL_PAGE_SIZE + m_slot_index * CLIENT_MEM_SIZE));

        if (repsonsePtr)
        {
            *response = repsonsePtr;
            m_slave_ready->post();
            return true;
        }
        
        m_slave_ready->post();
        return false;
    }

    template <typename Response>
    bool sendRequestGetResponse(const Message *request, const Response **response, size_t timeout_ms)
    {
        if (!isValid())
        {
            return false;
        }

        auto start = std::chrono::steady_clock::now();
        if (!sem_wait_timeout_adaptive(m_slave_ready.get(), timeout_ms))
        {
            return false;
        }

        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
        if (elapsed >= (long long)timeout_ms)
        {
            m_slave_ready->post();
            return false;
        }
        size_t remaining = timeout_ms - elapsed;

        // Drain any stale response signals
        while (m_slave_sent->tryWait()) {}

        // Set active slot in the control registry
        ClientSlotRegistry *registry = static_cast<ClientSlotRegistry*>(m_sender->getPtr());
        registry->active_slot.store(m_slot_index);

        m_sender->sendMessage(request, CONTROL_PAGE_SIZE + m_slot_index * CLIENT_MEM_SIZE);
        m_master_sent->post();

        if (!sem_wait_timeout_adaptive(m_slave_sent.get(), remaining))
        {
            m_slave_ready->post();
            return false;
        }

        const Response *repsonsePtr = static_cast<const Response *>(m_receiver->receiveMessage(CONTROL_PAGE_SIZE + m_slot_index * CLIENT_MEM_SIZE));

        if (repsonsePtr)
        {
            *response = repsonsePtr;
            m_slave_ready->post();
            return true;
        }
        
        m_slave_ready->post();
        return false;
    }

private:
    int m_slot_index{-1};
};
