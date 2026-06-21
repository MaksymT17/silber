#include <iostream>
#include <cstdlib>
#include <cstring>
#ifndef _WIN32
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#else
#include <windows.h>
#include <string>
#include <iostream>
#endif
#include "SharedMemoryReceiver.h"
#include <vector>

SharedMemoryReceiver::SharedMemoryReceiver(const char *shMemName) : m_name(shMemName)
{
    init();
}

SharedMemoryReceiver::~SharedMemoryReceiver()
{
    finish();
}
#ifndef _WIN32
void SharedMemoryReceiver::init()
{
    m_shm_fd = shm_open(m_name.c_str(), O_CREAT | O_EXCL | O_RDWR, 0666);
    if (m_shm_fd == -1)
    {
        if (errno == EEXIST)
        {
            m_shm_fd = shm_open(m_name.c_str(), O_RDWR, 0666);
            if (m_shm_fd == -1)
            {
                std::cerr << "SharedMemoryReceiver::init shm_open failed\n";
                perror("shm_open failed");
                m_ptr = nullptr;
                return;
            }
        }
        else
        {
            std::cerr << "SharedMemoryReceiver::init shm_open failed\n";
            m_ptr = nullptr;
            return;
        }
    }
    else
    {
        // Segment created successfully, truncate it to the desired size
        if (ftruncate(m_shm_fd, SHARED_MEMORY_SIZE) == -1)
        {
            std::cerr << "SharedMemoryReceiver::init ftruncate failed\n";
            m_ptr = nullptr;
            return;
        }
    }

    m_ptr = mmap(0, SHARED_MEMORY_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, m_shm_fd, 0);
    if (m_ptr == MAP_FAILED)
    {
        std::cerr << "SharedMemoryReceiver::init mmap failed\n";
        m_ptr = nullptr;
    }
}
void SharedMemoryReceiver::finish()
{
    if (m_ptr && m_ptr != (void*)-1 && m_ptr != MAP_FAILED)
    {
        if (munmap(m_ptr, SHARED_MEMORY_SIZE) == -1)
        {
            std::cerr << "munmap failed" << std::endl;
        }
        m_ptr = nullptr;
    }
    if (m_shm_fd != -1)
    {
        if (close(m_shm_fd) == -1)
        {
            std::cerr << "close failed" << std::endl;
        }
        m_shm_fd = -1;
    }
}
#else
void SharedMemoryReceiver::init()
{
    std::wstring wshMemName(m_name.begin(), m_name.end());
    m_shm_fd = OpenFileMappingW(
        FILE_MAP_ALL_ACCESS,
        FALSE,
        wshMemName.c_str());

    if (m_shm_fd == NULL)
    {
        m_shm_fd = CreateFileMappingW(
            INVALID_HANDLE_VALUE,
            NULL,
            PAGE_READWRITE,
            0,
            SHARED_MEMORY_SIZE,
            wshMemName.c_str());

        if (m_shm_fd == NULL)
        {
            printf(("Could not open file mapping object (%d).\n"),
                   GetLastError());
            return;
        }
    }

    m_ptr = (void *)MapViewOfFile(m_shm_fd,
                                  FILE_MAP_ALL_ACCESS,
                                  0,
                                  0,
                                  SHARED_MEMORY_SIZE);

    if (m_ptr == NULL)
    {
        printf("Could not map view of file (%d).\n", GetLastError());

        CloseHandle(m_shm_fd);

        return;
    }
}

void SharedMemoryReceiver::finish()
{
    if (m_ptr)
    {
        UnmapViewOfFile(m_ptr);
        m_ptr = nullptr;
    }
    if (m_shm_fd != NULL)
    {
        CloseHandle(m_shm_fd);
        m_shm_fd = NULL;
    }
}
#endif

Message *SharedMemoryReceiver::receiveMessage(const size_t offset)
{
    Message *message(static_cast<Message *>( (void*)(static_cast<char*>(m_ptr) + offset)) );
    return message;
}