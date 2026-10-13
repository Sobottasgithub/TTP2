#include "../include/peer_session_controller.h"

#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <stdexcept>

namespace ttp2 {
  PeerSessionController::PeerSessionController(std::string interface, int port) {
    std::cout << "PeerSessionController" << std::endl;

    std::string containerIP = SendSessionController::getLocalIpAddress(interface);

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = inet_addr(containerIP.c_str());

    int serverSocket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if(bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
        throw std::runtime_error("Bind failed!");
    }

    // Create epoll
    int epollFd = epoll_create1(0);
    if (epollFd == -1) {
        throw std::runtime_error("Failed to create epoll!");
    }
    // Set epoll action for server
    struct epoll_event serverEvents;
    serverEvents.events = EPOLLIN | EPOLLOUT;
    serverEvents.data.fd = serverSocket;
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, serverSocket, &serverEvents) == -1) {
        throw std::runtime_error("Failed to set epoll_ctl!");
    }

    this->serverSocket = serverSocket;
    this->epollFd = epollFd;
    this->port = port;
  }

  void PeerSessionController::connectTo(std::string ip) {
    connectTo(ip, this->port);
  }

  void PeerSessionController::connectTo(std::string ip, int port) {
    
  }

  std::vector<Packet> PeerSessionController::getReceivedPacketsFromIp(std::string ip) {
    
  }

  void PeerSessionController::sendPacketToIp(std::string ip) {
    
  }
  
  void PeerSessionController::serverAccpetCycle() {
    listen(serverSocket, 5);
    while (true) {
        const int MAX_EVENTS = 10;
        struct epoll_event events[MAX_EVENTS];
        int epollRequestCount = epoll_wait(epollFd, events, MAX_EVENTS, -1);
        
        for (int index = 0; index < epollRequestCount; ++index) {
            if (events[index].data.fd == serverSocket) {
                int clientSocket = accept4(serverSocket, nullptr, nullptr, SOCK_NONBLOCK);
                std::wcout << "New clientSocket: " << clientSocket << std::endl;

                peerConnections.push_back(std::thread(
                                               sendManager,
                                               clientSocket
                                            ));
            }
        }
    }
  }

  void PeerSessionController::sendManager(int clientSocket) {
    
  }

  void PeerSessionController::receiveManager() {
    
  }
}
