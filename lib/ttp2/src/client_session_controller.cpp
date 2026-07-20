#include "../include/client_session_controller.h"

#include <tablog.h>

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <thread>
#include <sys/epoll.h>

namespace ttp2 {
  ClientSessionController::ClientSessionController() {}

  ClientSessionController::ClientSessionController(int &socket) {
    this->socket = socket;
  }

  void ClientSessionController::networkingSession() {
    epollFd = epoll_create1(0);
    if (epollFd == -1) {
        logger->log(tablog::ERROR, "Failed to create epoll!");
    }

    serverEvent.events = EPOLLIN;
    serverEvent.data.fd = socket;
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, socket, &serverEvent) == -1) {
        logger->log(tablog::ERROR, "Failed to set epoll_ctl for client!");
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
    const int MAX_EVENTS = 10;

    while (isConnected()) {
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
          Packet packet = receiveMessage(fd);
          if (packet.id == -1) {
            continue;
          }
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

  void ClientSessionController::disconnect() {
    std::lock_guard<std::mutex> lock(mtx);
    close(this->socket);
    connected = false;
  }
}
