g++ -std=c++17 -O3 -g0 -mavx -Wall -Werror server.cpp -lpthread -lrt -o server
g++ -std=c++17 -O3 -g0 -mavx -Wall -Werror client.cpp -lpthread -lrt -o client
g++ -std=c++17 -O3 -g0 -mavx -Wall -Werror test.cpp -lpthread -lrt -o test
