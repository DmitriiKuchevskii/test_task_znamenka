#pragma once

#include "server.hpp"

class Client
{
private:
    Client(std::string recived_file_name, size_t size) :
        m_sh_mem_data(
            SharedMemory<char>::open(Server::SERVER_SHARED_MEMORY_DATA_NAME)
        ),
        m_sh_mem_sync_objs(
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
        std::cout << "Client will write its output into file '" << recived_file_name << "'\n";
        return std::shared_ptr<Client>(new Client(recived_file_name, broadcast_file_size));
    }

private:
    inline static size_t connect(bool start_broadcast)
    {
        auto sh_mem = SharedMemory<Server::ConnectData>::open(Server::SERVER_SHARED_MEMORY_CONNECT_DATA_NAME);
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
    inline void run()
    {
        std::ofstream file(m_recived_file_name, std::ios::binary);

        for (size_t i = 0; i < m_recived_file_size / m_sh_mem_data->size(); ++i)
            recive(file, m_sh_mem_data->size());

        size_t remain = m_recived_file_size % m_sh_mem_data->size();
        if (remain)
            recive(file, remain);
    }

private:
    inline void recive(std::ostream& file_stream, size_t size)
    {
        SharedMemorySyncObjects* sync = m_sh_mem_sync_objs->data();

        pthread_mutex_lock(&sync->mutex);
        {
            while(!sync->write_compleate)
                pthread_cond_wait(&sync->cond_var, &sync->mutex);
        }
        pthread_mutex_unlock(&sync->mutex);

        file_stream.write(m_sh_mem_data->data(), m_sh_mem_data->size());

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
    std::shared_ptr<SharedMemory<SharedMemorySyncObjects>> m_sh_mem_sync_objs;
    std::string m_recived_file_name;
    size_t m_recived_file_size;
};