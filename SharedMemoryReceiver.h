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
    void *getPtr() const { return m_ptr; }

private:
    HANDLE m_shm_fd;

    void *m_ptr;
    std::string m_name;
};
