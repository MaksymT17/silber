#include "ClientProcCommunicator.h"
#include "SlotRegistry.h"
#include <iostream>

ClientProcCommunicator::ClientProcCommunicator(
    const std::string &shMemName) : ProcCommunicator(shMemName)
{
    m_sender = std::make_unique<SharedMemorySender>(m_master_mem_name.c_str());
    m_receiver = std::make_unique<SharedMemoryReceiver>(m_slave_mem_name.c_str());

#ifndef _WIN32
    m_master_sent = sem_open(m_master_sent_s.c_str(), O_CREAT, 0666, N_SEM_OFF);
    m_slave_sent = sem_open(m_slave_sent_s.c_str(), O_CREAT, 0666, N_SEM_OFF);
    m_slave_ready = sem_open(m_slave_ready_s.c_str(), O_CREAT, 0666, N_SEM_ON);

    if (m_master_sent == SEM_FAILED || m_slave_sent == SEM_FAILED || m_slave_ready == SEM_FAILED)
    {
        std::cerr << "ProcCommunicator sem_open failure.\n";
    }
#else
    std::wstring wshMemName(shMemName.begin(), shMemName.end());

    m_master_sent = CreateSemaphoreW(NULL, N_SEM_OFF, MAXLONG, (wshMemName + L"_m_sent").c_str());
    m_slave_sent = CreateSemaphoreW(NULL, N_SEM_OFF, MAXLONG, (wshMemName + L"_s_sent").c_str());
    m_slave_ready = CreateSemaphoreW(NULL, N_SEM_ON, MAXLONG, (wshMemName + L"_s_ready").c_str());

    if (m_master_sent == NULL || m_slave_sent == NULL || m_slave_ready == NULL)
    {
        std::cerr << "ProcCommunicator sem_open failure.\n";
    }
#endif

    // Dynamic slot allocation
    m_slot_index = -1;
    if (m_sender && m_sender->isValid())
    {
        ClientSlotRegistry *registry = static_cast<ClientSlotRegistry*>(m_sender->getPtr());
        m_slot_index = SlotRegistry::claimSlot(registry);
    }

    if (m_slot_index == -1)
    {
        std::cerr << "ClientProcCommunicator warning: all communication slots are occupied or initialization failed.\n";
    }
}

bool ClientProcCommunicator::isValid() const
{
    if (m_slot_index == -1)
    {
        return false;
    }
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

ClientProcCommunicator::~ClientProcCommunicator()
{
    if (m_slot_index != -1 && m_sender && m_sender->isValid())
    {
        ClientSlotRegistry *registry = static_cast<ClientSlotRegistry*>(m_sender->getPtr());
        SlotRegistry::releaseSlot(registry, m_slot_index);
    }
}
