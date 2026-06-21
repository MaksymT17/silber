#include "ClientProcCommunicator.h"
#include "OSSemaphore.h"
#include "SlotRegistry.h"
#include <iostream>

ClientProcCommunicator::ClientProcCommunicator(const std::string &shMemName)
    : ClientProcCommunicator(
          shMemName,
          std::make_unique<SharedMemorySender>((shMemName + "_master").c_str()),
          std::make_unique<SharedMemoryReceiver>((shMemName + "_slave").c_str()))
{
}

ClientProcCommunicator::ClientProcCommunicator(
    const std::string &shMemName,
    std::unique_ptr<ISharedMemorySender> sender,
    std::unique_ptr<ISharedMemoryReceiver> receiver)
    : ProcCommunicator(shMemName, std::move(sender), std::move(receiver))
{
    m_master_sent = std::make_unique<OSSemaphore>(m_master_sent_s, true, N_SEM_OFF);
    m_slave_sent = std::make_unique<OSSemaphore>(m_slave_sent_s, true, N_SEM_OFF);
    m_slave_ready = std::make_unique<OSSemaphore>(m_slave_ready_s, true, N_SEM_ON);

    if (!m_master_sent->isValid() || !m_slave_sent->isValid() || !m_slave_ready->isValid())
    {
        std::cerr << "ProcCommunicator sem_open failure.\n";
    }

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
    return m_master_sent && m_slave_sent && m_slave_ready &&
           m_master_sent->isValid() && m_slave_sent->isValid() && m_slave_ready->isValid();
}

ClientProcCommunicator::~ClientProcCommunicator()
{
    if (m_slot_index != -1 && m_sender && m_sender->isValid())
    {
        ClientSlotRegistry *registry = static_cast<ClientSlotRegistry*>(m_sender->getPtr());
        SlotRegistry::releaseSlot(registry, m_slot_index);
    }
}
