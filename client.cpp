#include "client.hpp"

auto read_cmd_args(int argc, char** argv)
{
    bool start_broadcast = false;
    for (int i = 0; i < argc; ++i)
    {
        if (std::string(argv[i]) == "--start")
        {
            start_broadcast = true;
            break;
        }
    }
    std::string file_name = (argc >= 2 && std::string(argv[1]) != "--start") ? 
                            argv[1] :
                            std::to_string(getpid()) + ".txt";

    return std::make_tuple(start_broadcast, file_name);
}

int main(int argc, char** argv)
{
    try
    {
        auto[start_broadcast, file_name] = read_cmd_args(argc, argv);
        Client::create(file_name, start_broadcast)->run();
    }
    catch(const std::exception& e)
    {
        std::cout << e.what() << "\n";
        exit(EXIT_FAILURE);
    }
}