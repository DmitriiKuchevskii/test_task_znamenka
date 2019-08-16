#include "server.hpp"

int main(int argc, char** argv)
{
    try
    {
        if (argc < 2)
        {
            std::cout << "You must provide file name for broadcasting\n";
            exit(EXIT_FAILURE);
        }
        std::string file_name(argv[1]);
        if (!std::ifstream(file_name, std::ios::binary).good())
        {
            std::cout << "File " << file_name << "does not exist\n";
            exit(EXIT_FAILURE);
        }
        Server::create(file_name)->run();
    }
    catch(const std::exception& e)
    {
        std::cout << e.what() << "\n";
        exit(EXIT_FAILURE);
    }
}