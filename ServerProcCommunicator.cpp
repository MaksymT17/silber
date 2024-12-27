#include "ServerProcCommunicator.h"

ServerProcCommunicator::ServerProcCommunicator(
    const std::string &shMemName) : ProcCommunicator(shMemName)
{
    m_sender = std::make_unique<SharedMemorySender>(m_slave_mem_name.c_str());
    m_receiver = std::make_unique<SharedMemoryReceiver>(m_master_mem_name.c_str());

#ifndef _WIN32

    // Unlink semaphores to ensure fresh initialization
    sem_unlink(m_master_received_s.c_str());
    sem_unlink(m_slave_received_s.c_str());
    sem_unlink(m_master_sent_s.c_str());
    sem_unlink(m_slave_sent_s.c_str());
    sem_unlink(m_slave_ready_s.c_str());

    // Create semaphores with desired initial values
    m_master_received = sem_open(m_master_received_s.c_str(), O_CREAT, 0666, N_SEM_OFF);
    m_slave_received = sem_open(m_slave_received_s.c_str(), O_CREAT, 0666, N_SEM_OFF);
    m_master_sent = sem_open(m_master_sent_s.c_str(), O_CREAT, 0666, N_SEM_OFF);
    m_slave_sent = sem_open(m_slave_sent_s.c_str(), O_CREAT, 0666, N_SEM_OFF);
    m_slave_ready = sem_open(m_slave_ready_s.c_str(), O_CREAT, 0666, N_SEM_ON);

    if (m_master_received == SEM_FAILED || m_slave_received == SEM_FAILED ||
        m_master_sent == SEM_FAILED || m_slave_sent == SEM_FAILED || m_slave_ready == SEM_FAILED)
    {
        perror("sem_open failure");
        exit(EXIT_FAILURE);
    }

#else

    std::wstring wshMemName(shMemName.begin(), shMemName.end());

    // Create semaphores with desired initial values
    m_master_received = CreateSemaphoreW(NULL, N_SEM_OFF, MAXLONG, (wshMemName + L"_m_rsem").c_str());
    m_slave_received = CreateSemaphoreW(NULL, N_SEM_OFF, MAXLONG, (wshMemName + L"_s_rsem").c_str());
    m_master_sent = CreateSemaphoreW(NULL, N_SEM_OFF, MAXLONG, (wshMemName + L"_m_sent").c_str());
    m_slave_sent = CreateSemaphoreW(NULL, N_SEM_OFF, MAXLONG, (wshMemName + L"_s_sent").c_str());
    m_slave_ready = CreateSemaphoreW(NULL, N_SEM_ON, MAXLONG, (wshMemName + L"_s_ready").c_str());

    if (!m_master_received || !m_slave_received || !m_master_sent || !m_slave_sent || !m_slave_ready)
    {
        std::cerr << "CreateSemaphore failure, error: " << GetLastError() << '\n';
        exit(EXIT_FAILURE);
    }

#endif

}
ServerProcCommunicator::~ServerProcCommunicator()
{
#ifndef _WIN32
    if (sem_unlink(m_master_received_s.c_str()) == -1)
    {
        std::cerr <<"Failed to unlink m_master_received semaphore\n";
    }

    if (sem_unlink(m_slave_received_s.c_str()) == -1)
    {
        std::cerr <<"Failed to unlink m_slave_received semaphore\n";
    }

    if (sem_unlink(m_master_sent_s.c_str()) == -1)
    {
        std::cerr <<"Failed to unlink m_master_sent semaphore\n";
    }

    if (sem_unlink(m_slave_sent_s.c_str()) == -1)
    {
        std::cerr <<"Failed to unlink m_slave_sent semaphore\n";
    }

    if (sem_unlink(m_slave_ready_s.c_str()) == -1)
    {
        std::cerr <<"Failed to unlink m_slave_ready semaphore\n";
    }
#endif
}
#ifndef _WIN32

void ServerProcCommunicator::send(const Message *msg)
{
    m_sender->sendMessage(msg, msg->id * CLIENT_MEM_SIZE);
    sem_post(m_slave_sent);
    sem_wait(m_master_received);
    sem_post(m_slave_ready);
}

Message *ServerProcCommunicator::receive()
{
    sem_wait(m_master_sent);
    Message *response = m_receiver->receiveMessage();
    sem_post(m_slave_received);

    return response;
}

#else
void ServerProcCommunicator::send(const Message *msg)
{
    m_sender->sendMessage(msg, msg->id * CLIENT_MEM_SIZE);
    ReleaseSemaphore(m_slave_sent, 1, NULL);
    WaitForSingleObject(m_master_received, INFINITE);
    ReleaseSemaphore(m_slave_ready, 1, NULL);
}

Message *ServerProcCommunicator::receive()
{
    DWORD waitResult = WaitForSingleObject(m_master_sent, INFINITE);
    if (waitResult != WAIT_OBJECT_0)
    {
        std::cerr << "ProcCommunicator::receive WaitForSingleObject FAIL\n";
        return nullptr;
    }

    Message *response = m_receiver->receiveMessage();

    BOOL releaseResult = ReleaseSemaphore(m_slave_received, 1, NULL);
    if (!releaseResult)
    {
        std::cerr << "ProcCommunicator::receive ReleaseSemaphore FAIL\n";
        return nullptr;
    }
    return response;
}

#endif