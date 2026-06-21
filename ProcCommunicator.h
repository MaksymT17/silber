#pragma once

#include <memory>
#include <iostream>
#include "SharedMemorySender.h"
#include "SharedMemoryReceiver.h"
#include "Message.h"

inline void cpu_yield()
{
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    #if defined(_MSC_VER)
        // MSVC pauses
        #if !defined(__clang__)
            #include <intrin.h>
        #endif
        _mm_pause();
    #else
        __builtin_ia32_pause();
    #endif
#elif defined(__arm__) || defined(__aarch64__)
    __asm__ __volatile__("yield");
#endif
}

constexpr int N_SEM_OFF = 0;
constexpr int N_SEM_ON = 1;

class ProcCommunicator
{
protected:
    ProcCommunicator(const std::string &shMemName);
    virtual ~ProcCommunicator();

protected:
    std::unique_ptr<SharedMemorySender> m_sender;
    std::unique_ptr<SharedMemoryReceiver> m_receiver;

    const std::string m_master_received_s;
    const std::string m_slave_received_s;
    const std::string m_master_sent_s;
    const std::string m_slave_sent_s;
    const std::string m_slave_ready_s;

    const std::string m_master_mem_name;
    const std::string m_slave_mem_name;

    N_SEM m_master_received;
    N_SEM m_slave_received;
    N_SEM m_master_sent;
    N_SEM m_slave_sent;
    N_SEM m_slave_ready;
};