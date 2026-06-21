#include "ServerProcCommunicator.h"
#include "OSSemaphore.h"
#include "SilberLogging.h"

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
    // Unlink semaphores to ensure fresh initialization
    OSSemaphore::unlink(m_master_sent_s);
    OSSemaphore::unlink(m_slave_sent_s);
    OSSemaphore::unlink(m_slave_ready_s);

    // Create semaphores with desired initial values
    m_master_sent = std::make_unique<OSSemaphore>(m_master_sent_s, true, N_SEM_OFF);
    m_slave_sent = std::make_unique<OSSemaphore>(m_slave_sent_s, true, N_SEM_OFF);
    m_slave_ready = std::make_unique<OSSemaphore>(m_slave_ready_s, true, N_SEM_ON);

    if (!m_master_sent->isValid() || !m_slave_sent->isValid() || !m_slave_ready->isValid())
    {
        reportSilberError("ProcCommunicator sem_open failure.");
    }

    // Initialize/reset ClientSlotRegistry on startup to prevent stale process IDs
    if (m_receiver && m_receiver->isValid())
    {
        ClientSlotRegistry *registry = static_cast<ClientSlotRegistry*>(m_receiver->getPtr());
        if (registry)
        {
            for (size_t i = 0; i < MAX_CLIENTS_COUNT; ++i)
            {
                registry->slot_pids[i].store(0);
            }
            registry->active_slot.store(-1);
        }
    }
}

bool ServerProcCommunicator::isValid() const
{
    if (!m_sender || !m_receiver || !m_sender->isValid() || !m_receiver->isValid())
    {
        return false;
    }
    return m_master_sent && m_slave_sent && m_slave_ready &&
           m_master_sent->isValid() && m_slave_sent->isValid() && m_slave_ready->isValid();
}

ServerProcCommunicator::~ServerProcCommunicator()
{
    OSSemaphore::unlink(m_master_sent_s);
    OSSemaphore::unlink(m_slave_sent_s);
    OSSemaphore::unlink(m_slave_ready_s);

#ifndef _WIN32
    shm_unlink(m_master_mem_name.c_str());
    shm_unlink(m_slave_mem_name.c_str());
#endif
}

void ServerProcCommunicator::send(const Message *msg)
{
    if (!isValid()) return;
    ClientSlotRegistry *registry = static_cast<ClientSlotRegistry*>(m_receiver->getPtr());
    int32_t active_slot = registry->active_slot.load();

    m_sender->sendMessage(msg, CONTROL_PAGE_SIZE + active_slot * CLIENT_MEM_SIZE);
    m_slave_sent->post();
}

Message *ServerProcCommunicator::receive()
{
    if (!isValid()) return nullptr;
    
    // Adaptive spin wait
    bool acquired = false;
    for (int spin = 0; spin < 4000; ++spin)
    {
        if (m_master_sent->tryWait())
        {
            acquired = true;
            break;
        }
        cpu_yield();
    }
    if (!acquired)
    {
        m_master_sent->wait();
    }

    ClientSlotRegistry *registry = static_cast<ClientSlotRegistry*>(m_receiver->getPtr());
    int32_t active_slot = registry->active_slot.load();

    Message *response = m_receiver->receiveMessage(CONTROL_PAGE_SIZE + active_slot * CLIENT_MEM_SIZE);
    return response;
}