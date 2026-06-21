#pragma once

#include <stddef.h>
#include <vector>
#include <cstdint>
#include <atomic>

#ifndef _WIN32
#include <semaphore.h>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#else
#include <windows.h>
#endif

static constexpr size_t CLIENT_MEM_SIZE =  4096; // 4KB
static constexpr size_t MAX_CLIENTS_COUNT = 8; // 8 clients can interact with server
static constexpr size_t CONTROL_PAGE_SIZE = 4096; // 4KB for control data
static constexpr size_t SHARED_MEMORY_SIZE = CONTROL_PAGE_SIZE + MAX_CLIENTS_COUNT * CLIENT_MEM_SIZE;

struct ClientSlotRegistry
{
    std::atomic<uint32_t> slot_pids[MAX_CLIENTS_COUNT];
    std::atomic<int32_t> active_slot;
};

#ifndef _WIN32
using HANDLE = int;
using N_SEM = sem_t*;
#else
    using N_SEM = HANDLE;
#endif

enum MessageType : size_t
{
    HANDSHAKE = 0,
    HANDSHAKE_OK,
    HANDSHAKE_FAIL,
    SET_CONFIG,
    SET_CONFIG_OK,
    SET_CONFIG_FAIL,
    COMPARE_REQUEST,
    COMPARE_RESULT,
    COMPARE_FAIL,
    DISCONNECT,
    DISCONNECT_OK,
    DISCONNECT_FAIL,
    UNEXPECTED_REQUEST // default value, user must specify type of message
};

struct Message
{
    Message(const size_t aId = 0, const MessageType aType = MessageType::UNEXPECTED_REQUEST)
        : id(aId),
          type(aType),
          size(sizeof(Message)) {}
    size_t id;
    MessageType type;
    size_t size;
};