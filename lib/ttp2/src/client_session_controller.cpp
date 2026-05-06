#include "../include/client_session_controller.h"

#include "../include/methods.h"

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <thread>
#include <sys/epoll.h>

ClientSessionController::ClientSessionController() {}

ClientSessionController::ClientSessionController(int &socket) {
  this->socket = socket;
}

void ClientSessionController::networkingSession() {
  epollFd = epoll_create1(0);
  if (epollFd == -1) {
      std::wcout << "Failed to create epoll!" << std::endl;
  }

  serverEvent.events = EPOLLIN;
  serverEvent.data.fd = socket;
  if (epoll_ctl(epollFd, EPOLL_CTL_ADD, socket, &serverEvent) == -1) {
      std::wcout << "Failed to set epoll_ctl for client!" << std::endl;
      return;
  }

  std::thread sendRequestSessionThread([this]() {
      this->sendRequestSession();
  });

  std::thread receiveResponseSessionThread([this]() {
      this->receiveResponseSession();
  });
 
  while (isConnected()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }

  if (sendRequestSessionThread.joinable()) {
    sendRequestSessionThread.join();
  }

  if (receiveResponseSessionThread.joinable()) {
    receiveResponseSessionThread.join();
  }
}

void ClientSessionController::receiveResponseSession() {
  while (isConnected()) {
    const int MAX_EVENTS = 10;
    struct epoll_event incomingEvents[MAX_EVENTS];

    int eventCount = epoll_wait(epollFd, incomingEvents, MAX_EVENTS, -1);
    for (int index = 0; index < eventCount; ++index) {
        int fd = incomingEvents[index].data.fd;
        if (incomingEvents[index].events & EPOLLIN) {
          Packet packet = receiveMessage(socket);
          pushResponse(packet);
        }
    }
  }
}

void ClientSessionController::sendRequestSession() {
  while (isConnected()) {
    if (hasRequest()) {
      Packet request = popRequest();
      int responseCode = sendPacket(socket, request);
    }
  }
}
