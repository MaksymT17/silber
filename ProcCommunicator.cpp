#include "ProcCommunicator.h"
#include <iostream>
#include <cstring>

#ifndef _WIN32
#include <unistd.h>
#include <fcntl.h>
#endif

ProcCommunicator::ProcCommunicator(const std::string &shMemName) : m_master_mem_name(shMemName + "_master"),
                                                                   m_slave_mem_name(shMemName + "_slave"),
                                                                   m_master_sent_s(shMemName + "_m_sent"),
                                                                   m_slave_sent_s(shMemName + "_s_sent"),
                                                                   m_slave_ready_s(shMemName + "_s_ready")
{
}

ProcCommunicator::ProcCommunicator(const std::string &shMemName,
                                   std::unique_ptr<ISharedMemorySender> sender,
                                   std::unique_ptr<ISharedMemoryReceiver> receiver)
    : m_sender(std::move(sender)),
      m_receiver(std::move(receiver)),
      m_master_mem_name(shMemName + "_master"),
      m_slave_mem_name(shMemName + "_slave"),
      m_master_sent_s(shMemName + "_m_sent"),
      m_slave_sent_s(shMemName + "_s_sent"),
      m_slave_ready_s(shMemName + "_s_ready")
{
}
ProcCommunicator::ProcCommunicator(const std::string &shMemName,
                                   std::unique_ptr<ISharedMemorySender> sender,
                                   std::unique_ptr<ISharedMemoryReceiver> receiver,
                                   std::unique_ptr<ISemaphore> master_sent,
                                   std::unique_ptr<ISemaphore> slave_sent,
                                   std::unique_ptr<ISemaphore> slave_ready)
    : m_sender(std::move(sender)),
      m_receiver(std::move(receiver)),
      m_master_sent_s(shMemName + "_m_sent"),
      m_slave_sent_s(shMemName + "_s_sent"),
      m_slave_ready_s(shMemName + "_s_ready"),
      m_master_mem_name(shMemName + "_master"),
      m_slave_mem_name(shMemName + "_slave"),
      m_master_sent(std::move(master_sent)),
      m_slave_sent(std::move(slave_sent)),
      m_slave_ready(std::move(slave_ready))
{
}

ProcCommunicator::~ProcCommunicator()
{
}
