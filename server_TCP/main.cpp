#include "tcp_server.hpp"

int main() 
{
    // create server - it starts automatically in constructor
    TCPServer server;

    // wait for server thread to end
    server.join();

    return 0;
}
