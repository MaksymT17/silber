#include "SlotRegistry.h"

#ifndef _WIN32
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <cerrno>

bool SlotRegistry::isProcessAlive(uint32_t pid)
{
    if (pid == 0) return false;
    return (kill(pid, 0) == 0 || errno != ESRCH);
}

uint32_t SlotRegistry::getCurrentPid()
{
    return static_cast<uint32_t>(getpid());
}
#else
#include <windows.h>

bool SlotRegistry::isProcessAlive(uint32_t pid)
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

uint32_t SlotRegistry::getCurrentPid()
{
    return static_cast<uint32_t>(GetCurrentProcessId());
}
#endif

int SlotRegistry::claimSlot(ClientSlotRegistry *registry)
{
    if (!registry)
    {
        return -1;
    }
    uint32_t my_pid = getCurrentPid();
    for (size_t i = 0; i < MAX_CLIENTS_COUNT; ++i)
    {
        uint32_t expected_pid = registry->slot_pids[i].load();
        if (expected_pid == 0 || !isProcessAlive(expected_pid))
        {
            if (registry->slot_pids[i].compare_exchange_strong(expected_pid, my_pid))
            {
                return static_cast<int>(i);
            }
        }
    }
    return -1;
}

void SlotRegistry::releaseSlot(ClientSlotRegistry *registry, int slotIndex)
{
    if (registry && slotIndex >= 0 && slotIndex < static_cast<int>(MAX_CLIENTS_COUNT))
    {
        registry->slot_pids[slotIndex].store(0);
    }
}
