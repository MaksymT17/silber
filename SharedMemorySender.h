#pragma once

#include <string>
#include "Message.h"
#ifdef _WIN32
#include <windows.h>
#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#pragma comment(lib, "user32.lib")
#endif

class ISharedMemorySender
{
public:
    virtual ~ISharedMemorySender() = default;
    virtual void init() = 0;
    virtual void finish() = 0;
    virtual void sendMessage(const Message *msg, const size_t offset = 0) = 0;
    virtual void *getPtr() const = 0;
    virtual bool isValid() const = 0;
};

class SharedMemorySender : public ISharedMemorySender
{
public:
    SharedMemorySender(const char *shMemName);
    void init() override;
    void finish() override;
    void sendMessage(const Message *msg, const size_t offset=0) override;
    void *getPtr() const override { return m_ptr; }
    bool isValid() const override {
#ifndef _WIN32
        return m_ptr != nullptr && m_ptr != (void*)-1;
#else
        return m_ptr != nullptr;
#endif
    }

private:
    HANDLE m_shm_fd;
    void *m_ptr;
    std::string m_name;
};