cmake_minimum_required(VERSION 3.6.0)
project(cmake_s_sock VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)

add_executable(server_TCP main.cpp tcp_server.cpp connection_handler.cpp)

set (MY_INCLUDE_PATH ${CMAKE_SOURCE_DIR})


