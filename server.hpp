#pragma once

#include <fstream>

#include "shared_memory.hpp"

class Server
{
public:
    static constexpr char SERVER_SHARED_MEMORY_CONNECT_DATA_NAME[] = "/server_shared_memory_client_connect";
    struct ConnectData
    {
        inline void init()
        {
            mutex = get_ipc_mutex();
            cond_var = get_ipc_cond_var();
            clients = 0;
            start_broadcats = false;
            broadcast_file_size = 0;
            server_initialized = false;
        }
        unsigned clients;
        bool start_broadcats;
        pthread_mutex_t mutex;
        pthread_cond_t cond_var;
        size_t broadcast_file_size;
        bool server_initialized;
    };

public:
    static constexpr char SERVER_SHARED_MEMORY_DATA_NAME[] = "/server_shared_memory_data";
    static constexpr char SERVER_SHARED_MEMORY_SYNC_NAME[] = "/server_shared_memory_sync_objects";

    inline static std::shared_ptr<Server> create(
        const std::string& broadcast_file_name,
        size_t shared_memory_max_size = 1024 * 1024)
    {
        return std::shared_ptr<Server>(
            new Server(broadcast_file_name, shared_memory_max_size)
        );
    }

private:
    inline Server(const std::string& broadcast_file_name,
                  size_t shared_memory_max_size) :
        m_broadcast_file_name(std::move(broadcast_file_name))
    {
        unlink_shared_memory();

        m_sh_mem_data = SharedMemory<char>::create(
            SERVER_SHARED_MEMORY_DATA_NAME, shared_memory_max_size
        );
        m_sh_mem_sync = SharedMemory<SharedMemorySyncObjects>::create(
            SERVER_SHARED_MEMORY_SYNC_NAME
        );

        m_sh_mem_client_connect_data = SharedMemory<ConnectData>::create(
            SERVER_SHARED_MEMORY_CONNECT_DATA_NAME
        );
        m_sh_mem_client_connect_data->data()->init();

        auto sync = m_sh_mem_sync->data();
        sync->mutex = get_ipc_mutex();
        sync->cond_var = get_ipc_cond_var();
        sync->read_compleate = 0;
        sync->write_compleate = false;
    }

public:
    inline ~Server()
    {
        auto sync = m_sh_mem_sync->data();
        pthread_mutex_destroy(&sync->mutex);
        pthread_barrier_destroy(&sync->barier);
        pthread_cond_destroy(&sync->cond_var);

        unlink_shared_memory();
    }

    inline void run()
    {
        unsigned clients_number = wait_for_clients();
        m_sh_mem_sync->data()->barier = get_ipc_barier(clients_number + 1);

        std::ifstream file_stream(m_broadcast_file_name, std::ios::binary);
        size_t file_size = get_file_size(m_broadcast_file_name);

        for (size_t i = 0; i < file_size / m_sh_mem_data->size(); ++i)
            broadcast_data(file_stream, m_sh_mem_data->size(), clients_number);

        size_t remain = file_size % m_sh_mem_data->size();
        if (remain)
            broadcast_data(file_stream, remain, clients_number);
    }

private:
    inline void broadcast_data(std::istream& file_stream, size_t size, unsigned clients)
    {
        using namespace std::chrono;
        auto sync = m_sh_mem_sync->data();

        auto t1 = system_clock::now();
        file_stream.read(m_sh_mem_data->data(), size);
        auto t2 = system_clock::now();

        sync->time_before_write_into_shm = t1;
        sync->time_after_write_into_shm = t2;

        pthread_mutex_lock(&sync->mutex);
        {
            sync->write_compleate = true;
            pthread_cond_broadcast(&sync->cond_var);
            while(sync->read_compleate != clients)
                pthread_cond_wait(&sync->cond_var, &sync->mutex);
            sync->write_compleate = false;
            sync->read_compleate = 0;
        }
        pthread_mutex_unlock(&sync->mutex);

        pthread_barrier_wait(&sync->barier);
    }

    inline unsigned wait_for_clients()
    {
         auto client_connect_data = m_sh_mem_client_connect_data->data();
         std::cout << "Wating for clients...\n";
         unsigned clients = 0;
         pthread_mutex_lock(&client_connect_data->mutex);
         {
             client_connect_data->server_initialized = true;
             client_connect_data->broadcast_file_size = get_file_size(m_broadcast_file_name);
             pthread_cond_broadcast(&client_connect_data->cond_var);
             while(!client_connect_data->start_broadcats)
             {
                 pthread_cond_wait(&client_connect_data->cond_var, &client_connect_data->mutex);
                 if (clients != client_connect_data->clients)
                     LOGGER << "Client connected. (Clients: " << client_connect_data->clients << ")\n";
                 clients = client_connect_data->clients;
             }
         }
         pthread_mutex_unlock(&client_connect_data->mutex);
         LOGGER << client_connect_data->clients << " clients connected.\nStart broadcasting...\n";
         return client_connect_data->clients;
    }

    inline void unlink_shared_memory()
    {
        shm_unlink(SERVER_SHARED_MEMORY_DATA_NAME);
        shm_unlink(SERVER_SHARED_MEMORY_SYNC_NAME);
        shm_unlink(SERVER_SHARED_MEMORY_CONNECT_DATA_NAME);
    }

private:
    std::shared_ptr<SharedMemory<char>> m_sh_mem_data;
    std::shared_ptr<SharedMemory<SharedMemorySyncObjects>> m_sh_mem_sync;
    std::shared_ptr<SharedMemory<ConnectData>> m_sh_mem_client_connect_data;
    std::string m_broadcast_file_name;
};