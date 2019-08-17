#include "client.hpp"

constexpr char SHARED_MEMORY_TEST_SYNC_NAME[] = "/test_sync";
constexpr char TEST_BROADCAST_FILE_NAME[] = "test_broadcast_file.broadcast_data";
constexpr size_t TEST_BROADCAST_FILE_SIZE = 1024 * 1024 * 100;
constexpr unsigned TEST_CLIENTS_NUMBER = 10;

struct TestSync
{
    void init()
    {
        mutex = get_ipc_mutex();
        cond_var = get_ipc_cond_var();
        value = 0;
    }
    pthread_mutex_t mutex;
    pthread_cond_t cond_var;
    int value;
};

void init_test_shared_memory_sync()
{
    shm_unlink(SHARED_MEMORY_TEST_SYNC_NAME);
    auto sh_sync = SharedMemory<TestSync>::create(SHARED_MEMORY_TEST_SYNC_NAME);
    sh_sync->data()->init();
}

void wait_for_server_init()
{
    auto sh_mem_client_connect_data = SharedMemory<Server::ConnectData>::open(
        Server::SERVER_SHARED_MEMORY_CONNECT_DATA_NAME
    );
    auto client_connect_data = sh_mem_client_connect_data->data();
    pthread_mutex_lock(&client_connect_data->mutex);
    {
        while(!client_connect_data->server_initialized)
            pthread_cond_wait(&client_connect_data->cond_var, &client_connect_data->mutex);
    }
    pthread_mutex_unlock(&client_connect_data->mutex);
}

int run_client()
{
    wait_for_server_init();
    auto sh_sync = SharedMemory<TestSync>::open(SHARED_MEMORY_TEST_SYNC_NAME);
    auto sync = sh_sync->data();

    pthread_mutex_lock(&sync->mutex);
        int cur_val = ++sync->value;
        auto client = Client::create(std::to_string(cur_val) + ".broadcast_result",
                                    cur_val == TEST_CLIENTS_NUMBER);
    pthread_mutex_unlock(&sync->mutex);

    client->run();
    return 0;
}

std::string create_test_file(bool force_recreate = false)
{
    if (!force_recreate && std::ifstream(TEST_BROADCAST_FILE_NAME, std::ios::binary).good())
        return TEST_BROADCAST_FILE_NAME;

    std::ofstream file(TEST_BROADCAST_FILE_NAME, std::ios::binary);
    std::vector<char> data(TEST_BROADCAST_FILE_SIZE);
    for (size_t i = 0; i < data.size(); ++i)
        data[i] = char(i % std::numeric_limits<char>::max());
    file.write(data.data(), data.size());

    return TEST_BROADCAST_FILE_NAME;
}

bool equal_files(const std::string& first_file_name, const std::string& second_file_name)
{
    std::ifstream first(first_file_name, std::ios::binary);
    std::ifstream second(second_file_name, std::ios::binary);

    size_t first_size = get_file_size(first_file_name);
    size_t second_size = get_file_size(second_file_name);
    if (first_size != second_size)
      return false;

    std::vector<char> data1(first_size), data2(second_size);
    first.read(data1.data(), data1.size());
    second.read(data2.data(), data2.size());
    return data1 == data2;
}

void compare_files()
{
    for (size_t i = 1; i <= TEST_CLIENTS_NUMBER; ++i)
    {
        std::string result_file = std::to_string(i) + ".broadcast_result";
        if (!equal_files(result_file, TEST_BROADCAST_FILE_NAME))
            throw std::runtime_error(std::string("File ") + result_file + " is not identical to the test one.");
    }
}

auto remove_files()
{
    std::vector<std::string> unremoved_files;
    for (size_t i = 1; i <= TEST_CLIENTS_NUMBER; ++i)
    {
        std::string result_file = std::to_string(i) + ".broadcast_result";
        if (remove(result_file.c_str()))
            unremoved_files.push_back(result_file);
    }
    return unremoved_files;
}

int main(int argc, char** argv)
{
    try
    {
        init_test_shared_memory_sync();

        // Init server shared memory
        auto server = Server::create(create_test_file());

        // Run TEST_CLIENTS_NUMBER client process
        for (size_t i = 0; i < TEST_CLIENTS_NUMBER; ++i)
        {
            if (!fork())
            {
                return run_client();
            }
        }

        // Wait for clients and start broadcasting once recive "start" signal
        server->run();

        std::cout << "Broadcast completed.\nChecking results.....";
        compare_files();
        std::cout << "OK\nRemoving generated files.....";
        auto unremoved_files = remove_files();
        if (!unremoved_files.size())
        {
            std::cout << "All generated files have been succesfully removed.\n";
        }
        else
        {
            std::cout << "An error accured." 
                      << " Only " << TEST_CLIENTS_NUMBER - unremoved_files.size() 
                      << " out of" << TEST_CLIENTS_NUMBER << " have been removed. "
                      << "Can not remove the following files:\n";
            for (const auto& file : unremoved_files)
                std::cout << file << "\n";
        }
    }
    catch(const std::exception& e)
    {
        std::cout << e.what() << "\n";
        exit(EXIT_FAILURE);
    }
}