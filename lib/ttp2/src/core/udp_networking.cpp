#include "../../include/core/udp_networking.h"

#include <bits/stdc++.h> 
#include <stdexcept>
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <sys/epoll.h>
#include <netinet/in.h>

namespace ttp2 {
  void UdpNetworking::createServerSocket(const char *ip, int port) {
    struct sockaddr_in serverAddress; 
      
    if ((this->serverSocket = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0)) < 0) {
      throw std::runtime_error("Failed creating the server socket"); 
    }
      
    memset(&serverAddress, 0, sizeof(serverAddress)); 
      
    serverAddress.sin_family = AF_INET; 
    serverAddress.sin_addr.s_addr = inet_addr(ip); 
    serverAddress.sin_port = htons(port);
      
    if (bind(this->serverSocket,
        (const struct sockaddr *)&serverAddress,
        sizeof(serverAddress)) < 0 ) { 
      throw std::runtime_error("bind failed");
    }

    this->epollFd = epoll_create1(0);
    if (this->epollFd < 0) {
      close(this->serverSocket);
      throw std::runtime_error("epoll_create1 failed");
    }

    epoll_event event{};
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = this->serverSocket;
    if (epoll_ctl(this->epollFd, EPOLL_CTL_ADD, this->serverSocket, &event) < 0) {
      close(this->serverSocket);
      close(this->epollFd);
      throw std::runtime_error("epoll_ctl failed");
    }
  }

  std::string UdpNetworking::receivePacketFrom() {
    sockaddr_in clientAddress{};
    socklen_t clientLength = sizeof(clientAddress);
    char buffer[1024];
    ssize_t bytes = recvfrom(this->serverSocket, buffer, sizeof(buffer), 0, 
                             (struct sockaddr*)&clientAddress, &clientLength);

    std::cout << buffer << std::endl;
    return "";
  }

  void UdpNetworking::sendMessageTo(const char *ip, int port, const char *buffer) {
    int serverSocket;
    struct sockaddr_in serverAddress;

    serverSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (serverSocket < 0) {
      throw std::runtime_error("Socket creation failed");
    }

    memset(&serverAddress, 0, sizeof(serverAddress));

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = inet_addr(ip);

    sendto(serverSocket, buffer, strlen(buffer), MSG_CONFIRM,
           (const struct sockaddr *)&serverAddress, sizeof(serverAddress));

    std::cout << "SEND!" << std::endl;
  }
}
