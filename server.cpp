#include "server.hpp"

int main()
{
    try
    {
        Server::create("test_file.txt")->run();
    }
    catch(const std::exception& e)
    {
        std::cout << e.what() << "\n";
        exit(EXIT_FAILURE);
    }
}