#pragma once

#include "ServerProcCommunicator.h"
#include <thread>
#include <chrono>

static constexpr size_t WAIT_POLL_PERIOD = 5;

class ClientProcCommunicator : public ProcCommunicator
{
public:
    ClientProcCommunicator(const std::string &shMemName);

    virtual ~ClientProcCommunicator() = default;

#ifndef _WIN32
    /// note: blocking call, which will wait until server will repsond
    template <typename Response>
    bool sendRequestGetResponse(const Message *request, Response **reponse)
    {
        bool result{true};
        sem_wait(m_slave_ready);
        m_sender->sendMessage(request);
        sem_post(m_master_sent);
        sem_wait(m_slave_received);
        sem_wait(m_slave_sent);

        Response *repsonsePtr = static_cast<Response *>(m_receiver->receiveMessage(request->id * CLIENT_MEM_SIZE));

        if (repsonsePtr)
        {
            *reponse = repsonsePtr;
        }
        else
        {
            std::cerr << "ClientProcCommunicator::sendRequestGetResponse response type is not expected\n";
            result = false;
        }
        sem_post(m_master_received);
        return result;
    }

    /// note: non-blocking call, if response will be not provided method returns false.
    // Real-time computing implementation. User can skip Server answer if it is too late.
    // Next request could be sent instead.
    // Calculation time is not included in timeout_ms. Calculations timeout can be set via configuration.
    template <typename Response>
    bool sendRequestGetResponse(const Message *request, Response **reponse, size_t timeout_ms)
    {
        bool is_slave_ready{false};
        for (size_t i = 0; i <= timeout_ms; i += WAIT_POLL_PERIOD)
        {
            if (sem_trywait(m_slave_ready) == 0)
            {
                is_slave_ready = true;
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_POLL_PERIOD));
        }
        if (!is_slave_ready)
        {
            printf("is_slave_ready is not ready after the requested timeout %zu\n", timeout_ms);
            // release all semaphores, even if server respond later - it will be out of requested timebox
            sem_post(m_master_sent);
            sem_post(m_master_received);
            return false;
        }
        m_sender->sendMessage(request);
        sem_post(m_master_sent);
        sem_wait(m_slave_received);
        sem_wait(m_slave_sent);

        Response *repsonsePtr = static_cast<Response *>(m_receiver->receiveMessage(request->id * CLIENT_MEM_SIZE));

        if (repsonsePtr)
            *reponse = repsonsePtr;
        else
            std::cerr << "ClientProcCommunicator::sendRequestGetResponse response type is not expected\n";

        sem_post(m_master_received);
        return true;
    }

#else
    template <typename Response>
    bool sendRequestGetResponse(const Message *request, Response **reponse)
    {
        bool result{true};
        WaitForSingleObject(m_slave_ready, INFINITE);

        m_sender->sendMessage(request);
        ReleaseSemaphore(m_master_sent, 1, NULL);
        WaitForSingleObject(m_slave_received, INFINITE);
        WaitForSingleObject(m_slave_sent, INFINITE);

        Response *repsonsePtr = static_cast<Response *>(m_receiver->receiveMessage(request->id * CLIENT_MEM_SIZE));

        if (repsonsePtr)
        {
            *reponse = repsonsePtr;
        }
        else
        {
            std::cerr << "ClientProcCommunicator::sendRequestGetResponse response type is not expected\n";
            result = false;
        }

        ReleaseSemaphore(m_master_received, 1, NULL);
        return result;
    }
    template <typename Response>
    bool sendRequestGetResponse(const Message *request, Response **reponse, size_t timeout_ms)
    {
        DWORD result = WaitForSingleObject(m_slave_ready, timeout_ms);
        bool r_result{true};
        if (result == WAIT_TIMEOUT || result != WAIT_OBJECT_0)
        {
            std::cout << "is_slave_ready is not ready after the requested timeout: " << timeout_ms << std::endl;
            ReleaseSemaphore(m_master_sent, 1, NULL);
            ReleaseSemaphore(m_master_received, 1, NULL);
            return false;
        }

        m_sender->sendMessage(request);
        ReleaseSemaphore(m_master_sent, 1, NULL);
        WaitForSingleObject(m_slave_received, INFINITE);
        WaitForSingleObject(m_slave_sent, INFINITE);

        Response *repsonsePtr = static_cast<Response *>(m_receiver->receiveMessage(request->id * CLIENT_MEM_SIZE));

        if (repsonsePtr)
        {
            *reponse = repsonsePtr;
        }
        else
        {
            std::cerr << "ClientProcCommunicator::sendRequestGetResponse response type is not expected\n";
            r_result = false;
        }

        ReleaseSemaphore(m_master_received, 1, NULL);
        return r_result;
    }
#endif
};
