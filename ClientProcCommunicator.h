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
            *reponse = repsonsePtr;
        else{
            std::cerr << "ClientProcCommunicator::sendRequestGetResponse response type is not expected\n";
            result = false;
        }
        sem_post(m_master_received);
        return result;
    }

    /// note: non-blocking call, if response will be not provided method returns false.
    // Real-time computing implementation. User can skip Server answer if it is too late.
    // Next request could be sent instead.
    template <typename Response>
    bool sendRequestGetResponse(const Message *request, Response **reponse, size_t timeout_ms)
    {
        bool is_slave_ready{false};
        for (size_t i = 0; i <= timeout_ms; i += WAIT_POLL_PERIOD)
        {
            if (sem_trywait(m_slave_ready) == 0)
            {
                //printf("is_slave_ready acquired %zu\n", i);
                is_slave_ready = true;
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(WAIT_POLL_PERIOD));
        }
        if (!is_slave_ready)
        {
            printf("is_slave_ready is not ready after the requested timeout %zu\n", timeout_ms);
            // release all semaphores, even if server respond - it will be out requested timebox
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
            *reponse = repsonsePtr; // Response(*repsonsePtr);
        else
            std::cerr << "ClientProcCommunicator::sendRequestGetResponse response type is not expected\n";

        sem_post(m_master_received);
        return true;
    }

#else
    template <typename Response>
    void sendRequestGetResponse(const Message *request, Response **reponse)
    {
        WaitForSingleObject(m_slave_ready, INFINITE);

        m_sender->sendMessage(request);
        ReleaseSemaphore(m_master_sent, 1, NULL);
        WaitForSingleObject(m_slave_received, INFINITE);
        WaitForSingleObject(m_slave_sent, INFINITE);

        Response *repsonsePtr = static_cast<Response *>(m_receiver->receiveMessage(request->id * CLIENT_MEM_SIZE));

        if (repsonsePtr)
            *reponse = repsonsePtr;
        else
            std::cerr << "ClientProcCommunicator::sendRequestGetResponse response type is not expected\n";

        ReleaseSemaphore(m_master_received, 1, NULL);
    }
    template <typename Response>
    bool sendRequestGetResponse(const Message *request, Response **reponse, size_t timeout_ms)
    {
        DWORD result = WaitForSingleObject(m_slave_ready, timeout_ms);
        if (result == WAIT_TIMEOUT || result != WAIT_OBJECT_0)
        {
            printf("is_slave_ready is not ready after the requested timeout %zu result\n", timeout_ms, result);
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
            *reponse = repsonsePtr;
        else
            std::cerr << "ClientProcCommunicator::sendRequestGetResponse response type is not expected\n";

        ReleaseSemaphore(m_master_received, 1, NULL);
    }
#endif
};
