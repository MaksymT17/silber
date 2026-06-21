#include "ServerProcCommunicator.h"

#ifndef _WIN32
#include <sys/mman.h>
#include <unistd.h>
#endif

ServerProcCommunicator::ServerProcCommunicator(const std::string &shMemName)
    : ServerProcCommunicator(
          shMemName,
          std::make_unique<SharedMemorySender>((shMemName + "_slave").c_str()),
          std::make_unique<SharedMemoryReceiver>((shMemName + "_master").c_str()))
{
}

ServerProcCommunicator::ServerProcCommunicator(
    const std::string &shMemName,
    std::unique_ptr<ISharedMemorySender> sender,
    std::unique_ptr<ISharedMemoryReceiver> receiver)
    : ProcCommunicator(shMemName, std::move(sender), std::move(receiver))
{
#ifndef _WIN32

    // Unlink semaphores to ensure fresh initialization
    sem_unlink(m_master_sent_s.c_str());
    sem_unlink(m_slave_sent_s.c_str());
    sem_unlink(m_slave_ready_s.c_str());

    // Create semaphores with desired initial values
    m_master_sent = sem_open(m_master_sent_s.c_str(), O_CREAT, 0666, N_SEM_OFF);
    m_slave_sent = sem_open(m_slave_sent_s.c_str(), O_CREAT, 0666, N_SEM_OFF);
    m_slave_ready = sem_open(m_slave_ready_s.c_str(), O_CREAT, 0666, N_SEM_ON);

    if (m_master_sent == SEM_FAILED || m_slave_sent == SEM_FAILED || m_slave_ready == SEM_FAILED)
    {
        perror("sem_open failure");
    }

#else

    std::wstring wshMemName(shMemName.begin(), shMemName.end());

    // Create semaphores with desired initial values
    m_master_sent = CreateSemaphoreW(NULL, N_SEM_OFF, MAXLONG, (wshMemName + L"_m_sent").c_str());
    m_slave_sent = CreateSemaphoreW(NULL, N_SEM_OFF, MAXLONG, (wshMemName + L"_s_sent").c_str());
    m_slave_ready = CreateSemaphoreW(NULL, N_SEM_ON, MAXLONG, (wshMemName + L"_s_ready").c_str());

    if (!m_master_sent || !m_slave_sent || !m_slave_ready)
    {
        std::cerr << "CreateSemaphore failure, error: " << GetLastError() << '\n';
    }

#endif
}

bool ServerProcCommunicator::isValid() const
{
    if (!m_sender || !m_receiver || !m_sender->isValid() || !m_receiver->isValid())
    {
        return false;
    }
#ifndef _WIN32
    return m_master_sent != SEM_FAILED && m_slave_sent != SEM_FAILED && m_slave_ready != SEM_FAILED;
#else
    return m_master_sent != NULL && m_slave_sent != NULL && m_slave_ready != NULL;
#endif
}
ServerProcCommunicator::~ServerProcCommunicator()
{
#ifndef _WIN32
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

    shm_unlink(m_master_mem_name.c_str());
    shm_unlink(m_slave_mem_name.c_str());
#endif
}
#ifndef _WIN32

void ServerProcCommunicator::send(const Message *msg)
{
    if (!isValid()) return;
    ClientSlotRegistry *registry = static_cast<ClientSlotRegistry*>(m_receiver->getPtr());
    int32_t active_slot = registry->active_slot.load();

    m_sender->sendMessage(msg, CONTROL_PAGE_SIZE + active_slot * CLIENT_MEM_SIZE);
    sem_post(m_slave_sent);
}

Message *ServerProcCommunicator::receive()
{
    if (!isValid()) return nullptr;
    
    // Adaptive spin wait
    bool acquired = false;
    for (int spin = 0; spin < 4000; ++spin)
    {
        if (sem_trywait(m_master_sent) == 0)
        {
            acquired = true;
            break;
        }
        cpu_yield();
    }
    if (!acquired)
    {
        sem_wait(m_master_sent);
    }

    ClientSlotRegistry *registry = static_cast<ClientSlotRegistry*>(m_receiver->getPtr());
    int32_t active_slot = registry->active_slot.load();

    Message *response = m_receiver->receiveMessage(CONTROL_PAGE_SIZE + active_slot * CLIENT_MEM_SIZE);
    return response;
}

#else
void ServerProcCommunicator::send(const Message *msg)
{
    if (!isValid()) return;
    ClientSlotRegistry *registry = static_cast<ClientSlotRegistry*>(m_receiver->getPtr());
    int32_t active_slot = registry->active_slot.load();

    m_sender->sendMessage(msg, CONTROL_PAGE_SIZE + active_slot * CLIENT_MEM_SIZE);
    ReleaseSemaphore(m_slave_sent, 1, NULL);
}

Message *ServerProcCommunicator::receive()
{
    if (!isValid()) return nullptr;
    
    // Adaptive spin wait
    bool acquired = false;
    for (int spin = 0; spin < 4000; ++spin)
    {
        if (WaitForSingleObject(m_master_sent, 0) == WAIT_OBJECT_0)
        {
            acquired = true;
            break;
        }
        cpu_yield();
    }
    if (!acquired)
    {
        DWORD waitResult = WaitForSingleObject(m_master_sent, INFINITE);
        if (waitResult != WAIT_OBJECT_0)
        {
            std::cerr << "ProcCommunicator::receive WaitForSingleObject FAIL\n";
            return nullptr;
        }
    }

    ClientSlotRegistry *registry = static_cast<ClientSlotRegistry*>(m_receiver->getPtr());
    int32_t active_slot = registry->active_slot.load();

    Message *response = m_receiver->receiveMessage(CONTROL_PAGE_SIZE + active_slot * CLIENT_MEM_SIZE);
    return response;
}

#endif