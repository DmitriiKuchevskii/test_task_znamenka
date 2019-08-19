#pragma once

#include "server.hpp"

class BufferedWriteStream
{
public:
    inline BufferedWriteStream(const std::string& name) :
        m_file(fopen(name.c_str(), "wb")),
        m_buff(1024 * 1024 * 3) // 3 MB
    {}

    inline ~BufferedWriteStream()
    {
        if (m_cur)
            fwrite(m_buff.data(), sizeof(char), m_cur, m_file);
    }

    inline size_t write(const char* data, size_t size)
    {
        if (size + m_cur < m_buff.size())
        {
            memcpy(m_buff.data() + m_cur, data, size);
            m_cur += size;
            return size;
        }
        else
        {
            size_t written = fwrite(m_buff.data(), sizeof(char), m_cur, m_file);
            m_cur = 0;
            if (size < m_buff.size())
            {
                memcpy(m_buff.data(), data, size);
                m_cur = size;
                return m_cur + written;
            }
            else
            {
                return written + fwrite(data, sizeof(char), size, m_file);
            }
        }
    }

private:
    FILE* m_file = nullptr;
    std::vector<char> m_buff;
    size_t m_cur = 0;
};

class Client
{
private:
    inline Client(std::string recived_file_name, size_t size) :
        m_sh_mem_data(
            SharedMemory<char>::open(Server::SERVER_SHARED_MEMORY_DATA_NAME)
        ),
        m_sh_mem_sync(
            SharedMemory<SharedMemorySyncObjects>::open(Server::SERVER_SHARED_MEMORY_SYNC_NAME)
        ),
        m_recived_file_name(std::move(recived_file_name)),
        m_recived_file_size(size)
    {}

public:
    inline static std::shared_ptr<Client> create(const std::string& recived_file_name,
                                                 bool start_broadcast = false)
    {
        size_t broadcast_file_size = connect(start_broadcast);
        std::cout << "Client connected.\nBroadcast " << (!start_broadcast ? "not " : "") << "started.\n";
        if (!start_broadcast)
            std::cout << "Should you want to run broadcasting, you need to provide --start option for client.\n";
        LOGGER << "Client will write its output into file '" << recived_file_name << "'\n";
        return std::shared_ptr<Client>(new Client(recived_file_name, broadcast_file_size));
    }

private:
    inline static size_t connect(bool start_broadcast)
    {
        auto sh_mem = SharedMemory<Server::ConnectData>::open(
            Server::SERVER_SHARED_MEMORY_CONNECT_DATA_NAME
        );
        auto client_connect_data = sh_mem->data();
        pthread_mutex_lock(&client_connect_data->mutex);
        {
            client_connect_data->clients++;
            client_connect_data->start_broadcats = start_broadcast;
            pthread_cond_signal(&client_connect_data->cond_var);
        }
        pthread_mutex_unlock(&client_connect_data->mutex);
        return client_connect_data->broadcast_file_size;
    }

public:
    inline auto run()
    {
        BufferedWriteStream file(m_recived_file_name);
        size_t sh_mem_data_size = m_sh_mem_data->size();

        for (size_t i = 0; i < m_recived_file_size / sh_mem_data_size; ++i)
            recive(file, sh_mem_data_size);

        size_t remain = m_recived_file_size % sh_mem_data_size;
        if (remain)
            recive(file, remain);

        return m_latency_stat;
    }

private:
    inline void recive(auto& file_stream, size_t size)
    {
        using namespace std::chrono;
        auto sync = m_sh_mem_sync->data();

        pthread_mutex_lock(&sync->mutex);
        {
            while(!sync->write_compleate)
                pthread_cond_wait(&sync->cond_var, &sync->mutex);
        }
        pthread_mutex_unlock(&sync->mutex);

        auto t1 = system_clock::now();
        file_stream.write(m_sh_mem_data->data(), size);
        auto t2 = system_clock::now();

        auto t0_1 = sync->time_before_write_into_shm;
        auto t0_2 = sync->time_after_write_into_shm;
        size_t l1 = duration_cast<microseconds>(t2 - t0_1).count();
        size_t l2 = duration_cast<microseconds>(t1 - t0_2).count();
        m_latency_stat.push_back({l1, l2});

        pthread_mutex_lock(&sync->mutex);
        {
            sync->read_compleate++;
            pthread_cond_signal(&sync->cond_var);
        }
        pthread_mutex_unlock(&sync->mutex);

        pthread_barrier_wait(&sync->barier);
    }

private:
    std::shared_ptr<SharedMemory<char>> m_sh_mem_data;
    std::shared_ptr<SharedMemory<SharedMemorySyncObjects>> m_sh_mem_sync;
    std::string m_recived_file_name;
    size_t m_recived_file_size;
    std::vector<std::pair<size_t, size_t>> m_latency_stat;
};