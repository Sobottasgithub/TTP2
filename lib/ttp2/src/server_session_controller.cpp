#include "../include/server_session_controller.h"

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <thread>
#include <sys/epoll.h>

namespace ttp2 {
  ServerSessionController::ServerSessionController() {}

  ServerSessionController::ServerSessionController(int serverSocket, int clientSocket) {
    this->serverSocket = serverSocket;
    this->clientSocket = clientSocket;
  }

  void ServerSessionController::networkingSession() {
    epollFd = epoll_create1(0);
    if (epollFd == -1) {
        std::wcout << "Failed to create epoll!" << std::endl;
    }

    clientEvent.events = EPOLLIN;
    clientEvent.data.fd = clientSocket;
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, clientSocket, &clientEvent) == -1) {
        std::wcout << "Failed to set epoll_ctl for client!" << std::endl;
        return;
    }

    std::thread receiveRequestSessionThread([this]() {
        this->receiveRequestSession();
    });

    std::thread sendResponseSessionThread([this]() {
        this->sendResponseSession();
    });
 
    while (isConnected()) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    if (receiveRequestSessionThread.joinable()) {
      receiveRequestSessionThread.join();
    }

    if (sendResponseSessionThread.joinable()) {
      sendResponseSessionThread.join();
    }
  }

  void ServerSessionController::sendResponseSession() {
    while (isConnected()) {
      if (hasResponse()) {
        Packet responsePacket = popResponse();
        int responseCode = sendPacket(clientSocket, responsePacket);
      }
    }
  }

  void ServerSessionController::receiveRequestSession() {
    const int MAX_EVENTS = 10;
    while (isConnected()) {
      struct epoll_event incomingEvents[MAX_EVENTS];
      int eventCount = epoll_wait(epollFd, incomingEvents, MAX_EVENTS, -1);
    
      for (int index = 0; index < eventCount; ++index) {
        int fd = incomingEvents[index].data.fd;
        if (incomingEvents[index].events & (EPOLLHUP | EPOLLERR)) {
          sessionBuffers.erase(fd);
          close(fd);
          disconnect();
          continue;
        }
        if (incomingEvents[index].events & EPOLLIN) {
          Packet packet = receiveMessage(fd);
          if (packet.id == -1) {
            continue;
          }
          pushRequest(packet);
        }
      }
    }
  }
  
  void ServerSessionController::disconnect() {
    std::lock_guard<std::mutex> lock(mtx);
    close(this->clientSocket);
    connected = false;
  }
}
