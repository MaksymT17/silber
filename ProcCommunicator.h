#pragma once

#include <memory>
#include <iostream>
#include "SharedMemorySender.h"
#include "SharedMemoryReceiver.h"
#include "Message.hpp"
#ifndef _WIN32
#include <semaphore.h>
#else
#include <windows.h>
#endif


class ProcCommunicator
{
public:
    ProcCommunicator(const bool isMasterMode, const bool isMultipleMasters, const std::string &shMemName);
    ~ProcCommunicator();

    void send(const Message *msg);
    Message *receive();
    //void sendRequestGetResponse(const Message *request, Message &reponse);
    void ackNotify();

    template <typename Response>
    void sendRequestGetResponse(const Message *request, Response &reponse)
    {
        if (m_master_mode)
        {
            // send
            sem_wait(m_slave_ready);
            m_sender->sendMessage(request);
            sem_post(m_master_mode ? m_master_sent : m_slave_sent);
            sem_wait(m_master_mode ? m_slave_received : m_master_received);
            // receive
            sem_wait(m_master_mode ? m_slave_sent : m_master_sent);
            Response *repsonsePtr = static_cast<Response *>(m_receiver->receiveMessage());

            if (repsonsePtr)
                reponse = *repsonsePtr;
            else
                std::cerr << "ProcCommunicator::sendRequestGetResponse response type is not expected\n";

            sem_post(m_master_mode ? m_master_received : m_slave_received);
            // release slave for next messages
            sem_post(m_slave_ready);
        }
        else
        {
            std::cerr << "ProcCommunicator::sendRequestGetResponse allowed only from clients\n";
        }
    }

private:
    std::unique_ptr<SharedMemorySender> m_sender;
    std::unique_ptr<SharedMemoryReceiver> m_receiver;
    bool m_master_mode;
    
#ifndef _WIN32
    sem_t *m_master_received;
    sem_t *m_slave_received;
    sem_t *m_master_sent;
    sem_t *m_slave_sent;
    sem_t *m_slave_ready;
#else
    HANDLE m_master_received;
    HANDLE m_slave_received;
    HANDLE m_master_sent;
    HANDLE m_slave_sent;
    HANDLE m_slave_ready;
#endif
};