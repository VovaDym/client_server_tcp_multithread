#pragma once

#include <thread>
#include <mutex>
#include <fstream>

#define BUFLEN 512

class ConnectionHandler {
private:
    std::thread threadClient;
    std::mutex mtx;
    std::ofstream out_file; 
    int fd = -1;
    bool terminateThread = false;
    
    std::string readMessage();
    
    std::string timeStamp();

    void stop();

public:
    explicit ConnectionHandler(int fd);
    ~ConnectionHandler();

    void terminate();
    void threadFunc();
};