#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <chrono>
#include <string>
#include <iostream>

#define SERVER_NAME "127.0.0.1"

void sendMessage(const std::string& name, int fd);

int main(void)
{
  int sock;
  int err;
  uint16_t server_port;
  int period;
  struct sockaddr_in server_addr; 
  struct hostent *hostinfo;
  std::string name;

  hostinfo = gethostbyname(SERVER_NAME);
  if(hostinfo == nullptr)
  {
    std::cout << stderr << "Unknown host" << std::endl;
    exit(EXIT_FAILURE);
  }
  std::cout << "Enter the name: ";
  std::cin >> name;
  std::cout << "Enter the port number: ";
  std::cin >> server_port;
  std::cout << "Enter the connection period in seconds: ";
  std::cin >> period;
  server_addr.sin_family = PF_INET;
  server_addr.sin_port = htons(server_port);
  server_addr.sin_addr = *(struct in_addr*) hostinfo->h_addr;

  while (true)
  {
    sock = socket(PF_INET,SOCK_STREAM,0);
    if (sock < 0)
    {
      perror("Client: socket was not created");
      exit(EXIT_FAILURE);
    }
     
    err = connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr));    
    if (err < 0)
    {
      std::cout <<"Client: cannot connect socket" << std::endl;
      exit (EXIT_FAILURE);
    }
  
    sendMessage(name, sock);
  
    sleep(period);
    shutdown(sock, 2);
    close(sock); 
  }  
}

void sendMessage(const std::string& name, int fd)
{
    int n = send(fd, name.c_str(), name.size() + 1, 0);
    if (n != name.size() + 1) 
    {
        std::cout << "Error while sending message, message size: " 
                  << name.size() << " bytes sent: " << std::endl;
    }
}
