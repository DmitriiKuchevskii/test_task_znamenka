#include <algorithm>

#include "client.hpp"

constexpr char SHARED_MEMORY_TEST_SYNC_NAME[] = "/test_sync";
constexpr char TEST_BROADCAST_FILE_NAME[] = "test_broadcast_file.broadcast_data";
constexpr size_t TEST_MESSAGE_SIZE = 1024 * 1024;
constexpr size_t TEST_BROADCAST_FILE_SIZE = 1024 * 1024 * 100;
unsigned TEST_CLIENTS_NUMBER = 0;

struct TestSync
{
    void init()
    {
        mutex = get_ipc_mutex();
        cond_var = get_ipc_cond_var();
        barier = get_ipc_barier(TEST_CLIENTS_NUMBER + 1);
        value = 0;
        sh_mem_initialized = false;
    }
    pthread_mutex_t mutex;
    pthread_cond_t cond_var;
    pthread_barrier_t barier;
    int value;
    bool sh_mem_initialized;
};
std::shared_ptr<SharedMemory<TestSync>> g_test_sync_ptr;
TestSync* g_test_sync;

void init_test_shared_memory_sync()
{
    shm_unlink(SHARED_MEMORY_TEST_SYNC_NAME);
    g_test_sync_ptr = SharedMemory<TestSync>::create(SHARED_MEMORY_TEST_SYNC_NAME);
    g_test_sync = g_test_sync_ptr->data();
    g_test_sync->init();
}

void wait_for_server_shared_memory_init()
{
    pthread_mutex_lock(&g_test_sync->mutex);
        while(!g_test_sync->sh_mem_initialized)
            pthread_cond_wait(&g_test_sync->cond_var, &g_test_sync->mutex);
    pthread_mutex_unlock(&g_test_sync->mutex);
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
    wait_for_server_shared_memory_init();
    wait_for_server_init();

    pthread_mutex_lock(&g_test_sync->mutex);
        unsigned cur_val = ++g_test_sync->value;
        auto client = Client::create(std::to_string(cur_val) + ".broadcast_result",
                                    cur_val == TEST_CLIENTS_NUMBER);
    pthread_mutex_unlock(&g_test_sync->mutex);

    auto latency_stat = client->run();
    size_t percentile_90th = size_t(0.1 * latency_stat.size());
    std::sort(std::begin(latency_stat), std::end(latency_stat), [](const auto& e1, const auto& e2)
    {
        return e1.first < e2.first;
    });
    size_t l1 = latency_stat[percentile_90th].first;
    std::sort(std::begin(latency_stat), std::end(latency_stat), [](const auto& e1, const auto& e2)
    {
        return e1.second < e2.second;
    });
    size_t l2 = latency_stat[percentile_90th].second;
    pthread_mutex_lock(&g_test_sync->mutex);
        std::cout
        << "---------------------------------------------------------------------------------------------------------\n"
        << "Client's(PID: " << getpid() << ") 90th percentile statistic:\n"
        << "Latecy_1 (FROM Server started read chunk of data TO Client finished write chunk of data): " << l1 << " microsec\n"
        << "Latecy_2 (FROM Server finished write into SHM TO Client started read from SHM): " << l2 << " microsec\n"
        << "---------------------------------------------------------------------------------------------------------\n";
    pthread_mutex_unlock(&g_test_sync->mutex);

    // Wait for all reports displayed
    pthread_barrier_wait(&g_test_sync->barier);
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

void test_results()
{
     // Wait for all reports displayed
    pthread_barrier_wait(&g_test_sync->barier);
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

int main(int argc, char** argv)
{
    try
    {
        TEST_CLIENTS_NUMBER = argc < 2 ? 10 : std::stoi(argv[1]);
        std::cout << "Test with " << TEST_CLIENTS_NUMBER << " clients\n"
                  << "Broadcasting " << 1.0 * TEST_BROADCAST_FILE_SIZE / (1024 * 1024) << " MB\n"
                  << "Message size is " << 1.0 * TEST_MESSAGE_SIZE / (1024 * 1024) << " MB\n";
        init_test_shared_memory_sync();

        // We want to create a server in parent process
        // and all clients in child processes
        std::shared_ptr<Server> server;

        // Run TEST_CLIENTS_NUMBER client process (child processes)
        for (size_t i = 0; i < TEST_CLIENTS_NUMBER; ++i)
        {
            if (!fork())
            {
                return run_client();
            }
        }

        // Init shared memory (parent process)
        server = Server::create(create_test_file(), TEST_MESSAGE_SIZE);

        //Signal child processes that they can access server's shared memory
        pthread_mutex_lock(&g_test_sync->mutex);
            g_test_sync->sh_mem_initialized = true;
            pthread_cond_broadcast(&g_test_sync->cond_var);
        pthread_mutex_unlock(&g_test_sync->mutex);

        // Wait for clients and start broadcasting
        // once recive "start" signal from client
        server->run();

        // Check if produced files are binary same compare to server's input file
        test_results();
    }
    catch(const std::invalid_argument&)
    {
        std::cout << "Number of clients is incorrect\n";
        exit(EXIT_FAILURE);
    }
    catch(const std::exception& e)
    {
        std::cout << e.what() << "\n";
        exit(EXIT_FAILURE);
    }
}