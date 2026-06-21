#include <cstdlib>
#include <cstring>
#ifndef _WIN32
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#endif
#include <string>
#include <exception>
#include "SharedMemorySender.h"
#include "SilberLogging.h"

SharedMemorySender::SharedMemorySender(const char *shMemName) : m_name(shMemName)
{
    init();
}

SharedMemorySender::~SharedMemorySender()
{
    finish();
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
                reportSilberError("SharedMemorySender::init shm_open failed");
                m_ptr = nullptr;
                return;
            }
        }
        else
        {
            reportSilberError("SharedMemorySender::init shm_open failed");
            m_ptr = nullptr;
            return;
        }
    }
    else
    {
        if (ftruncate(m_shm_fd, SHARED_MEMORY_SIZE) == -1)
        {
            reportSilberError("SharedMemorySender::init ftruncate failed");
            m_ptr = nullptr;
            return;
        }
    }

    // Map the shared memory object into the address space of the process
    m_ptr = mmap(0, SHARED_MEMORY_SIZE, PROT_WRITE, MAP_SHARED, m_shm_fd, 0);
    if (m_ptr == MAP_FAILED)
    {
        reportSilberError("mmap failed");
        m_ptr = nullptr;
    }
}

void SharedMemorySender::finish()
{
    if (m_ptr && m_ptr != (void*)-1 && m_ptr != MAP_FAILED)
    {
        if (munmap(m_ptr, SHARED_MEMORY_SIZE) == -1)
        {
            reportSilberError("munmap failed");
        }
        m_ptr = nullptr;
    }
    if (m_shm_fd != -1)
    {
        if (close(m_shm_fd) == -1)
        {
            reportSilberError("close failed");
        }
        m_shm_fd = -1;
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
        reportSilberError("Could not create file mapping object (%d).", GetLastError());
    }
    m_ptr = (void *)MapViewOfFile(m_shm_fd, FILE_MAP_ALL_ACCESS, 0, 0, SHARED_MEMORY_SIZE);

    if (m_ptr == NULL)
    {
        reportSilberError("Could not map view of file (%d).", GetLastError());
        CloseHandle(m_shm_fd);
    }
}

void SharedMemorySender::finish()
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

void SharedMemorySender::sendMessage(const Message *msg, const size_t offset)
{
    CopyMemory(static_cast<char *>(m_ptr) + offset, msg, msg->size);
}
#endif
