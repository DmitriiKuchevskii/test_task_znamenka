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
        if (!std::ifstream(argv[1], std::ios::binary).good())
        {
            std::cout << "File " << argv[1] << "does not exist\n";
            exit(EXIT_FAILURE);
        }
        Server::create(argv[0])->run();
    }
    catch(const std::exception& e)
    {
        std::cout << e.what() << "\n";
        exit(EXIT_FAILURE);
    }
}