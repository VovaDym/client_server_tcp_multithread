#include "tcp_server.hpp"
#include <unistd.h>
#include <poll.h>
#include <sys/eventfd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cassert>
#include <vector>
#include <unordered_map>
#include "connection_handler.hpp"
#include <iostream>

TCPServer::TCPServer(): efd(-1) 
{   
    std::cout << "Enter server number port: " << std::endl;
    std::cin >> PORT;

    this->start();
}

TCPServer::~TCPServer() 
{
    this->stop();
    if (this->efd != -1)
    {
        close(this->efd);
    }
}

void TCPServer::start() 
{
    assert(!this->threadServer.joinable());

    if (this->efd != -1) 
    {
        close(this->efd);
    }

    this->efd = eventfd(0, 0);
    if (this->efd == -1) 
    {
        std::cout << "eventfd() => -1, errno = " << errno << std::endl;
        return;
    }

    // creates thread
    this->threadServer = std::thread([this]() { this->threadFunc(); });
}


void TCPServer::stop() 
{
    // writes to efd - it will be handled as stopping server thread
    uint64_t one = 1;
    auto ret = write(this->efd, &one, 8);
    if (ret == -1) 
    {
        std::cout << "write => -1, errno = " << errno << std::endl;
    }
}

void TCPServer::join() 
{
    if (this->threadServer.joinable()) 
    {
        this->threadServer.join();
    }
}

void TCPServer::threadFunc() 
{
    int sock;

    std::cout << "Listen on: " << PORT << std::endl;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) 
    {
        std::cout << "socket() => -1, errno = " << errno << std::endl;
        return;
    }

    int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int)) == -1) 
    {
        std::cout << "setsockopt() => -1, errno = " << errno << std::endl;
    }

    struct sockaddr_in servaddr = {0,0,0,0};
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(PORT);

    // binding to socket that will listen for new connections
    if (bind(sock, (const struct sockaddr*)&servaddr, sizeof(servaddr)) == -1) 
    {
        std::cout << "bind() => -1, errno = " << errno << std::endl;
        close(sock);
        return;
    }

    // started listening, 50 pending connections can wait in a queue
    listen(sock, 50);

    // monitored file descriptors - at start there is efd and just created sockfd. POLLIN means we wait for data to read
    std::vector<struct pollfd> fds{ { this->efd, POLLIN, 0 }, { sock, POLLIN, 0 } };
    
    std::unordered_map<int, ConnectionHandler> handlers;

    while (true) 
    {
        const int TIMEOUT = 1000;   // 1000 ms
        int n = poll(fds.data(), fds.size(), TIMEOUT);  // checking if there was any event on monitored file descriptors
        if (n == -1 && errno != ETIMEDOUT && errno != EINTR) 
        {
            std::cout << "poll() => -1, errno = " << errno << std::endl;
            break;
        }
        
        // n pending events
        if (n > 0) 
        {
            if (fds[0].revents) 
            {   // handles server stop request (which is sent by TCPServer::stop())
                std::cout << "Received stop request" << std::endl;
                break;
            } else if (fds[1].revents) 
            {    // new client connected
                // accepting connection
                int clientSock = accept(sock, NULL, NULL);
                std::cout << "New connection" << std::endl;
                if (clientSock != -1) 
                {
                    // insert new pollfd to monitor
                    fds.push_back(pollfd{clientSock, POLLIN, 0});

                    // create ConnectionHandler object that will run in separate thread
                    handlers.emplace(clientSock, clientSock);
                } else 
                {
                    std::cout << "accept => -1, errno=" << errno << std::endl;
                }

                // clearing revents
                fds[1].revents = 0;
            }
            
            // iterating all pollfds to check if anyone disconnected
            for (auto it = fds.begin() + 2; it != fds.end(); ) 
            {
                char c;
                // checks if disconnected or just fd readable
                if (it->revents && recv(it->fd, &c, 1, MSG_PEEK | MSG_DONTWAIT) == 0) 
                { 
                    std::cout << "Client disconnected" << std::endl;
                    close(it->fd);  // closing socket
                    handlers.at(it->fd).terminate();    // terminating ConnectionHandler thread
                    handlers.erase(it->fd);
                    it = fds.erase(it);
                } else 
                {
                    ++it;
                }
            }
        }
    }

    // cleaning section after receiving stop request
    for (auto it = fds.begin() + 1; it != fds.end(); it++) 
    {
        close(it->fd);
        if (handlers.find(it->fd) != handlers.end()) 
        {
            handlers.at(it->fd).terminate();
        }
    }
}