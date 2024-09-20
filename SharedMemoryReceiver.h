#pragma once

#include <string>
#include "Message.h"

class SharedMemoryReceiver
{
public:
    SharedMemoryReceiver(const char *shMemName);
    void init();
    void finish();
    Message *receiveMessage(const size_t offset = 0);

private:
#ifndef _WIN32
    int m_shm_fd;
#else
    HANDLE m_shm_fd;
#endif

    void *m_ptr;
    std::string m_name;
};
