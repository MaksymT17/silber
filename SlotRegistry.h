#pragma once

#include <cstdint>
#include "Message.h"

class SlotRegistry
{
public:
    // Claims a communication slot in the shared memory registry.
    // Returns the slot index (0 to MAX_CLIENTS_COUNT-1) on success, or -1 on failure.
    static int claimSlot(ClientSlotRegistry *registry);

    // Releases the claimed slot in the shared memory registry.
    static void releaseSlot(ClientSlotRegistry *registry, int slotIndex);

private:
    static bool isProcessAlive(uint32_t pid);
    static uint32_t getCurrentPid();
};
