#include "ClientProcCommunicator.h"
#include <iostream>

#ifndef _WIN32
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>

static bool is_process_alive(uint32_t pid)
{
    if (pid == 0) return false;
    return (kill(pid, 0) == 0 || errno != ESRCH);
}

static uint32_t get_current_pid()
{
    return static_cast<uint32_t>(getpid());
}
#else
#include <windows.h>

static bool is_process_alive(uint32_t pid)
{
    if (pid == 0) return false;
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (process)
    {
        DWORD exitCode;
        if (GetExitCodeProcess(process, &exitCode))
        {
            CloseHandle(process);
            return exitCode == STILL_ACTIVE;
        }
        CloseHandle(process);
    }
    return false;
}

static uint32_t get_current_pid()
{
    return static_cast<uint32_t>(GetCurrentProcessId());
}
#endif

ClientProcCommunicator::ClientProcCommunicator(
    const std::string &shMemName) : ProcCommunicator(shMemName)
{
    m_sender = std::make_unique<SharedMemorySender>(m_master_mem_name.c_str());
    m_receiver = std::make_unique<SharedMemoryReceiver>(m_slave_mem_name.c_str());

#ifndef _WIN32
    m_master_received = sem_open(m_master_received_s.c_str(), O_CREAT, 0666, N_SEM_OFF);
    m_slave_received = sem_open(m_slave_received_s.c_str(), O_CREAT, 0666, N_SEM_OFF);
    m_master_sent = sem_open(m_master_sent_s.c_str(), O_CREAT, 0666, N_SEM_OFF);
    m_slave_sent = sem_open(m_slave_sent_s.c_str(), O_CREAT, 0666, N_SEM_OFF);
    m_slave_ready = sem_open(m_slave_ready_s.c_str(), O_CREAT, 0666, N_SEM_ON);

    if (m_master_received == SEM_FAILED || m_slave_received == SEM_FAILED ||
        m_master_sent == SEM_FAILED || m_slave_sent == SEM_FAILED || m_slave_ready == SEM_FAILED)
    {
        std::cerr << "ProcCommunicator sem_open failure.\n";
    }
#else
    std::wstring wshMemName(shMemName.begin(), shMemName.end());

    m_master_received = CreateSemaphoreW(NULL, N_SEM_OFF, MAXLONG, (wshMemName + L"_m_rsem").c_str());
    m_slave_received = CreateSemaphoreW(NULL, N_SEM_OFF, MAXLONG, (wshMemName + L"_s_rsem").c_str());
    m_master_sent = CreateSemaphoreW(NULL, N_SEM_OFF, MAXLONG, (wshMemName + L"_m_sent").c_str());
    m_slave_sent = CreateSemaphoreW(NULL, N_SEM_OFF, MAXLONG, (wshMemName + L"_s_sent").c_str());
    m_slave_ready = CreateSemaphoreW(NULL, N_SEM_ON, MAXLONG, (wshMemName + L"_s_ready").c_str());

    if (m_master_received == NULL || m_slave_received == NULL ||
        m_master_sent == NULL || m_slave_sent == NULL || m_slave_ready == NULL)
    {
        std::cerr << "ProcCommunicator sem_open failure.\n";
    }
#endif

    // Dynamic slot allocation
    m_slot_index = -1;
    if (m_sender && m_sender->isValid())
    {
        ClientSlotRegistry *registry = static_cast<ClientSlotRegistry*>(m_sender->getPtr());
        if (registry)
        {
            uint32_t my_pid = get_current_pid();
            for (size_t i = 0; i < MAX_CLIENTS_COUNT; ++i)
            {
                uint32_t expected_pid = registry->slot_pids[i].load();
                if (expected_pid == 0 || !is_process_alive(expected_pid))
                {
                    if (registry->slot_pids[i].compare_exchange_strong(expected_pid, my_pid))
                    {
                        m_slot_index = static_cast<int>(i);
                        break;
                    }
                }
            }
        }
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
    return m_master_received != SEM_FAILED && m_slave_received != SEM_FAILED &&
           m_master_sent != SEM_FAILED && m_slave_sent != SEM_FAILED && m_slave_ready != SEM_FAILED;
#else
    return m_master_received != NULL && m_slave_received != NULL &&
           m_master_sent != NULL && m_slave_sent != NULL && m_slave_ready != NULL;
#endif
}

ClientProcCommunicator::~ClientProcCommunicator()
{
    if (m_slot_index != -1 && m_sender && m_sender->isValid())
    {
        ClientSlotRegistry *registry = static_cast<ClientSlotRegistry*>(m_sender->getPtr());
        if (registry)
        {
            registry->slot_pids[m_slot_index].store(0);
        }
    }
}
