#pragma once

#include <thread>
#include <fstream>
#include <iostream>
#include <vector>
#include <cstring>
#include <chrono>

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <semaphore.h>
#include <unistd.h>

#ifdef _ENABLE_LOGGING_
#define LOGGER std::cout
#else
std::ofstream __null_stream__;
#define LOGGER __null_stream__
#endif

inline size_t get_file_size(const std::string& file_name)
{
    std::ifstream file_stream(file_name, std::ios::binary);
    file_stream.seekg(0, std::ios::end);
    size_t file_size = file_stream.tellg();
    file_stream.seekg(0, std::ios::beg);
    return file_size;
}

inline pthread_mutex_t get_ipc_mutex()
{
    pthread_mutex_t mutex;
    pthread_mutexattr_t mutex_attr;
    pthread_mutexattr_init(&mutex_attr);
    pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&mutex, &mutex_attr);
    return mutex;
}

inline pthread_cond_t get_ipc_cond_var()
{
    pthread_cond_t cond_var;
    pthread_condattr_t cond_attr;
    pthread_condattr_init(&cond_attr);
    pthread_condattr_setpshared(&cond_attr, PTHREAD_PROCESS_SHARED);
    pthread_cond_init(&cond_var, &cond_attr);
    return cond_var;
}

inline pthread_barrier_t get_ipc_barier(unsigned value)
{
    pthread_barrier_t barier;
    pthread_barrierattr_t barier_attr;
    pthread_barrierattr_init(&barier_attr);
    pthread_barrierattr_setpshared(&barier_attr, PTHREAD_PROCESS_SHARED);
    pthread_barrier_init(&barier, &barier_attr, value);
    return barier;
}

struct SharedMemorySyncObjects
{
    pthread_mutex_t mutex;
    pthread_cond_t  cond_var;
    pthread_barrier_t barier;
    bool write_compleate;
    unsigned read_compleate;
    std::chrono::time_point<std::chrono::system_clock> time_before_write_into_shm;
    std::chrono::time_point<std::chrono::system_clock> time_after_write_into_shm;
};

template <typename T>
class SharedMemory
{
public:
    class SharedMemoryException : public std::exception
    {
    public:
        inline SharedMemoryException(std::string message) :
            m_message(std::move(message))
        {
            m_message += "\nError code: " + std::to_string(errno);
        }

        inline const char* what() const noexcept override
        {
            return m_message.c_str();
        }

    private:
        std::string m_message;
    };

    inline static std::shared_ptr<SharedMemory<T>> create(const std::string& name, size_t size = 1)
    {
        return std::shared_ptr<SharedMemory<T>>(new SharedMemory<T>(name, size,
            O_RDWR | O_CREAT, S_IRWXU, PROT_READ | PROT_WRITE, MAP_SHARED));
    }

    inline static std::shared_ptr<SharedMemory<T>> open(const std::string& name)
    {
        return std::shared_ptr<SharedMemory<T>>(new SharedMemory<T>(name, -1,
            O_RDWR, S_IRWXU, PROT_READ | PROT_WRITE, MAP_SHARED));
    }

private:
    inline SharedMemory(const std::string& name, size_t size,
                        int oflags, int omode, int mflags, int mmode) :
        m_size(size),
        m_mflags(mflags),
        m_mmode(mmode)
    {
        m_sh_mem_descriptor = shm_open(name.c_str(), oflags, omode);

        if (m_sh_mem_descriptor == -1)
            throw SharedMemoryException("Can not open shared memory object.");

        if ((oflags & O_CREAT) == O_CREAT)
        {
            if (ftruncate(m_sh_mem_descriptor, m_size * sizeof(T)) == -1)
                throw SharedMemoryException("Can not allocate shared memory.");
        }
        else
        {
            struct stat mem_info;
            if (fstat(m_sh_mem_descriptor, &mem_info))
                throw SharedMemoryException("Can not get shared memory object info.");
            m_size = mem_info.st_size;
        }

        m_data = (T*)mmap(NULL, m_size * sizeof(T), mflags, mmode, m_sh_mem_descriptor, 0);
        if (m_data == nullptr)
            throw SharedMemoryException("Can not map shared memory to virtual sapce.");
    }

public:
    inline ~SharedMemory()
    {
        close(m_sh_mem_descriptor);
        munmap(m_data, m_size);
    }

    inline T* data()
    {
        return m_data;
    }

    inline const T* data() const
    {
        return m_data;
    }

    inline size_t size() const
    {
        return m_size;
    }

private:
    size_t m_size = 0;
    T* m_data = nullptr;
    int m_sh_mem_descriptor = -1;
    int m_mflags = 0, m_mmode = 0;
};