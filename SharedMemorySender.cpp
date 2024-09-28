#include <iostream>
#include <cstdlib>
#include <cstring>
#ifndef _WIN32
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#endif
#include <string>
#include <iostream>
#include <exception>
#include "SharedMemorySender.h"

SharedMemorySender::SharedMemorySender(const char *shMemName) : m_name(shMemName)
{
    init();
}
#ifndef _WIN32
void SharedMemorySender::init()
{
    m_shm_fd = shm_open(m_name.c_str(), O_CREAT | O_EXCL | O_RDWR, 0666);
    if (m_shm_fd == -1)
    {
        if (errno == EEXIST)
        {
            m_shm_fd = shm_open(m_name.c_str(), O_RDWR, 0666);
            if (m_shm_fd == -1)
            {
                std::cerr << "SharedMemorySender::init shm_open failed" << std::endl;
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            std::cerr << "SharedMemorySender::init shm_open failed" << std::endl;
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        if (ftruncate(m_shm_fd, SHARED_MEMORY_SIZE) == -1)
        {
            std::cerr << "SharedMemorySender::init ftruncate failed" << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    // Map the shared memory object into the address space of the process
    m_ptr = mmap(0, SHARED_MEMORY_SIZE, PROT_WRITE, MAP_SHARED, m_shm_fd, 0);
    if (m_ptr == MAP_FAILED)
    {
        std::cerr << "mmap failed" << std::endl;
        throw std::exception();
    }
}

void SharedMemorySender::finish()
{
    if (munmap(m_ptr, SHARED_MEMORY_SIZE) == -1)
    {
        std::cerr << "munmap failed" << std::endl;
    }
    if (close(m_shm_fd) == -1)
    {
        std::cerr << "close failed" << std::endl;
    }

    if (shm_unlink(m_name.c_str()) == -1)
    {
        std::cerr << "shm_unlink failed" << std::endl;
    }
}

void SharedMemorySender::sendMessage(const Message *msg, const size_t offset)
{
    std::memcpy(static_cast<char *>(m_ptr) + offset, msg, msg->size);
}

#else
void SharedMemorySender::init()
{
    std::wstring wshMemName(m_name.begin(), m_name.end());
    m_shm_fd = CreateFileMappingW(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, SHARED_MEMORY_SIZE, wshMemName.c_str());

    if (m_shm_fd == NULL)
    {
        printf("Could not create file mapping object (%d).\n",
               GetLastError());
    }
    m_ptr = (void *)MapViewOfFile(m_shm_fd, FILE_MAP_ALL_ACCESS, 0, 0, SHARED_MEMORY_SIZE);

    if (m_ptr == NULL)
    {
        printf("Could not map view of file (%d).\n", GetLastError());
        CloseHandle(m_shm_fd);
    }
}

void SharedMemorySender::finish()
{
    UnmapViewOfFile(m_ptr);
    CloseHandle(m_shm_fd);
}

void SharedMemorySender::sendMessage(const Message *msg, const size_t offset)
{
    CopyMemory(static_cast<char *>(m_ptr) + offset, msg, msg->size);
}
#endif
