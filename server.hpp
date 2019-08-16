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
        }
        unsigned clients;
        bool start_broadcats;
        pthread_mutex_t mutex;
        pthread_cond_t cond_var;
        size_t broadcast_file_size;
    };

public:
    static constexpr char SERVER_SHARED_MEMORY_DATA_NAME[] = "/server_shared_memory_data";
    static constexpr char SERVER_SHARED_MEMORY_SYNC_NAME[] = "/server_shared_memory_sync_objects";

public:
    inline static std::shared_ptr<Server> create(const std::string& broadcast_file_name, 
                                                 size_t shared_memory_max_size = 1024 * 1024)
    {
        unlink_shared_memory();

        auto sh_mem_data = SharedMemory<char>::create(SERVER_SHARED_MEMORY_DATA_NAME, shared_memory_max_size);
        auto sh_mem_sync = SharedMemory<SharedMemorySyncObjects>::create(SERVER_SHARED_MEMORY_SYNC_NAME);
        SharedMemorySyncObjects* sync = sh_mem_sync->data();
        sync->mutex = get_ipc_mutex();
        sync->cond_var = get_ipc_cond_var();
        sync->read_compleate = 0;
        sync->write_compleate = false;

        unsigned clients_number = wait_for_clients(get_file_size(broadcast_file_name));
        sync->barier = get_ipc_barier(clients_number);

        return std::shared_ptr<Server>(
            new Server(broadcast_file_name, clients_number, sh_mem_data, sh_mem_sync)
        );
    }

private:
    inline static unsigned wait_for_clients(size_t file_size)
    {
         auto sh_mem_clients = SharedMemory<ConnectData>::create(SERVER_SHARED_MEMORY_CONNECT_DATA_NAME);
         ConnectData* client_connect_data = sh_mem_clients->data();
         client_connect_data->init();
         std::cout << "Wating for clients...\n";
         unsigned clients = 0;
         pthread_mutex_lock(&client_connect_data->mutex);
         {
             client_connect_data->broadcast_file_size = file_size;
             while(!client_connect_data->start_broadcats)
             {
                 pthread_cond_wait(&client_connect_data->cond_var, &client_connect_data->mutex);
                 if (clients != client_connect_data->clients)
                     std::cout << "Client connected. (Clients: " << client_connect_data->clients << ")\n";
                 clients = client_connect_data->clients;
             }
         }
         pthread_mutex_unlock(&client_connect_data->mutex);
         std::cout << client_connect_data->clients << " clients connected.\nStart broadcasting...\n";
         return client_connect_data->clients;
    }
    
    static inline void unlink_shared_memory()
    {
        shm_unlink(SERVER_SHARED_MEMORY_DATA_NAME);
        shm_unlink(SERVER_SHARED_MEMORY_SYNC_NAME);
        shm_unlink(SERVER_SHARED_MEMORY_CONNECT_DATA_NAME);
    }

    inline Server(std::string broadcast_file_name,
                  unsigned clients_number,
                  std::shared_ptr<SharedMemory<char>> sh_mem_data,
                  std::shared_ptr<SharedMemory<SharedMemorySyncObjects>> sh_mem_sync) :
        m_sh_mem_data(sh_mem_data),
        m_sh_mem_sync(sh_mem_sync),
        m_clients_number(clients_number),
        m_broadcast_file_name(std::move(broadcast_file_name))
    {}

public:
    inline ~Server()
    {
        SharedMemorySyncObjects* sync = m_sh_mem_sync->data();
        pthread_mutex_destroy(&sync->mutex);
        pthread_barrier_destroy(&sync->barier);
        pthread_cond_destroy(&sync->cond_var);

        unlink_shared_memory();
    }

    inline void run()
    {
        std::ifstream file_stream(m_broadcast_file_name, std::ios::binary);
        size_t file_size = get_file_size(m_broadcast_file_name);

        for (size_t i = 0; i < file_size / m_sh_mem_data->size(); ++i)
        {
            broadcast_data(file_stream, m_sh_mem_data->size());
        }

        size_t remain = file_size % m_sh_mem_data->size();
        if (remain)
        {
            m_sh_mem_data->resize(remain);
            broadcast_data(file_stream, remain);
        }
    }

private:

    inline void broadcast_data(std::istream& file_stream, size_t size)
    {
        SharedMemorySyncObjects* sync = m_sh_mem_sync->data();
        file_stream.read(m_sh_mem_data->data(), size);

        pthread_mutex_lock(&sync->mutex);
        {
            sync->write_compleate = true;
            pthread_cond_broadcast(&sync->cond_var);
            while(sync->read_compleate != m_clients_number)
                pthread_cond_wait(&sync->cond_var, &sync->mutex);
            sync->write_compleate = false;
            sync->read_compleate = 0;
        }
        pthread_mutex_unlock(&sync->mutex);

        pthread_barrier_wait(&sync->barier);
    }

private:
    std::shared_ptr<SharedMemory<char>> m_sh_mem_data;
    std::shared_ptr<SharedMemory<SharedMemorySyncObjects>> m_sh_mem_sync;
    unsigned m_clients_number = 0;
    std::string m_broadcast_file_name;
};