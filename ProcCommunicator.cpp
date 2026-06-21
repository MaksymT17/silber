#include "ProcCommunicator.h"
#include <iostream>
#include <cstring>

#ifndef _WIN32
#include <unistd.h>
#include <fcntl.h>
#endif

ProcCommunicator::ProcCommunicator(const std::string &shMemName) : m_master_mem_name(shMemName + "_master"),
                                                                   m_slave_mem_name(shMemName + "_slave"),
                                                                   m_master_received_s(shMemName + "_m_rsem"),
                                                                   m_slave_received_s(shMemName + "_s_rsem"),
                                                                   m_master_sent_s(shMemName + "_m_sent"),
                                                                   m_slave_sent_s(shMemName + "_s_sent"),
                                                                   m_slave_ready_s(shMemName + "_s_ready")
{
}
ProcCommunicator::~ProcCommunicator()
{
#ifndef _WIN32
    if (m_master_received != SEM_FAILED && sem_close(m_master_received) == -1)
    {
        std::cerr << "Failed to close m_master_received semaphore: " << strerror(errno) << '\n';
    }

    if (m_slave_received != SEM_FAILED && sem_close(m_slave_received) == -1)
    {
        std::cerr << "Failed to close m_slave_received semaphore: " << strerror(errno) << '\n';
    }

    if (m_master_sent != SEM_FAILED && sem_close(m_master_sent) == -1)
    {
        std::cerr << "Failed to close m_master_sent semaphore: " << strerror(errno) << '\n';
    }

    if (m_slave_sent != SEM_FAILED && sem_close(m_slave_sent) == -1)
    {
        std::cerr << "Failed to close m_slave_sent semaphore: " << strerror(errno) << '\n';
    }

    if (m_slave_ready != SEM_FAILED && sem_close(m_slave_ready) == -1)
    {
        std::cerr << "Failed to close m_slave_ready semaphore: " << strerror(errno) << '\n';
    }

#else
    if (m_master_received != NULL && !CloseHandle(m_master_received))
    {
        std::cerr << "Failed to close m_master_received semaphore, error: " << GetLastError() << '\n';
    }

    if (m_slave_received != NULL && !CloseHandle(m_slave_received))
    {
        std::cerr << "Failed to close m_slave_received semaphore, error: " << GetLastError() << '\n';
    }

    if (m_master_sent != NULL && !CloseHandle(m_master_sent))
    {
        std::cerr << "Failed to close m_master_sent semaphore, error: " << GetLastError() << '\n';
    }

    if (m_slave_sent != NULL && !CloseHandle(m_slave_sent))
    {
        std::cerr << "Failed to close m_slave_sent semaphore, error: " << GetLastError() << '\n';
    }

    if (m_slave_ready != NULL && !CloseHandle(m_slave_ready))
    {
        std::cerr << "Failed to close m_slave_ready semaphore, error: " << GetLastError() << '\n';
    }
#endif
}
