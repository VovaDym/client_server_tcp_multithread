#include "connection_handler.hpp"
#include <cassert>
#include <sys/socket.h>
#include <errno.h>
#include <iostream>
#include <pthread.h>
#include <fstream>
#include <string.h>
#include <iosfwd>
#include <chrono>
#include <ctime>   
#include <sstream> 
#include <iomanip> 

std::string ConnectionHandler::timeStamp()
{   
    const auto current_time_point {std::chrono::system_clock::now()};
    const auto current_time {std::chrono::system_clock::to_time_t (current_time_point)};
    const auto current_localtime {*std::localtime (&current_time)};
    const auto current_time_since_epoch {current_time_point.time_since_epoch()};
    const auto current_milliseconds {std::chrono::duration_cast<std::chrono::milliseconds> 
                                    (current_time_since_epoch).count() % 1000};
    
    std::ostringstream stream;
    stream << std::put_time (&current_localtime, "%Y-%m-%d %X") << "." 
           << std::setw (3) << std::setfill ('0') << current_milliseconds;
    return stream.str();
}

ConnectionHandler::ConnectionHandler(int fd) : fd(fd) 
{
    assert(!this->threadClient.joinable());

    // creating thread that handles received messages
    this->threadClient = std::thread([this]() { this->threadFunc(); });
}

ConnectionHandler::~ConnectionHandler() 
{
    this->stop();
}

void ConnectionHandler::stop() 
{
    if (this->threadClient.joinable()) 
    {
        this->threadClient.join();
    }
}

void ConnectionHandler::threadFunc() 
{
    if (!this->terminateThread) 
    {
        std::string msg = this->readMessage();
        std::cout << "The client connected by name: " << msg << std::endl;
        
        mtx.lock();   
        out_file.open("log.txt",std::ios::out|std::ios::app);      
        if (out_file.is_open())
        {
           out_file << "["<< timeStamp() <<"]"<< msg << std::endl;
        }
        out_file.close(); 
        std::cout << "File has been written" << std::endl;
        mtx.unlock(); 
    }     
}

std::string ConnectionHandler::readMessage() 
{
    std::string msg(BUFLEN, '\0'); // buffor with 512 length which is filled with NULL character
    
    int readBytes = recv(this->fd, (void*)msg.data(), sizeof(msg), 0);
    if (readBytes < 1) 
    {
        std::cout << "Error in readMessage, readBytes: " << readBytes << std::endl;
        return "";
    }
    std::string sub_msg = msg.substr(0,readBytes - 1);
    return sub_msg;
}

void ConnectionHandler::terminate() 
{
    this->terminateThread = true;
}
