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

class SharedMemorySender
{
public:
    SharedMemorySender(const char *shMemName);
    void init();
    void finish();
    void sendMessage(const Message *msg, const size_t offset=0);
    void *getPtr() const { return m_ptr; }
    bool isValid() const {
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