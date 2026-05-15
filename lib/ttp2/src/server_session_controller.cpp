#include "../include/server_session_controller.h"

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <thread>
#include <sys/epoll.h>

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

std::string ServerSessionController::getLocalIpAddress(std::string interface) {
  struct ifaddrs *ifaddr = nullptr;

  // Get linked list of network interfaces
  if (getifaddrs(&ifaddr) == -1) {
      return "";
  }

  std::string result;

  // Iterate through interfaces
  for (auto *ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
    if (!ifa->ifa_addr) continue;

    if (ifa->ifa_addr->sa_family == AF_INET) {
      auto *addr = reinterpret_cast<struct sockaddr_in *>(ifa->ifa_addr);
      char ip[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, &addr->sin_addr, ip, sizeof(ip));

      // Docker containers typically use eth0
      if (std::string(ifa->ifa_name) == interface) {
          result = ip;
          break;
      }
    }
  }

  freeifaddrs(ifaddr);
  return result;
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
  while (isConnected()) {
    const int MAX_EVENTS = 10;
    struct epoll_event incomingEvents[MAX_EVENTS];

    int eventCount = epoll_wait(epollFd, incomingEvents, MAX_EVENTS, -1);
    
    for (int index = 0; index < eventCount; ++index) {
      int fd = incomingEvents[index].data.fd;
      if (incomingEvents[index].events & (EPOLLHUP | EPOLLERR)) {
        sessionBuffers.erase(fd);
        close(fd);
        continue;
      }
      if (incomingEvents[index].events & EPOLLIN) {
        while (true) {
          Packet packet = receiveMessage(fd);
          if (packet.id == -1) {
            break;
          }
          pushRequest(packet);
        }
      }
    }
  }
}

