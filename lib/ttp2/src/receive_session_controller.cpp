#include "../include/receive_session_controller.h"

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/epoll.h>

namespace ttp2 {
  ReceiveSessionController::ReceiveSessionController(std::string ipV4, int port) {
    configureLogger("TTP2");
    // WARNING: ::socket is used here because otherwise it would interfere with (int) this->socket
    this->socket = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = inet_addr(ipV4.c_str());

    int connectionResult = connect(this->socket, (struct sockaddr*) &serverAddress, sizeof(serverAddress));
    
    if (connectionResult < 0 && errno != EINPROGRESS) {
        logger->log(tablog::ERROR, "Connection failed!");
    }
  }
  
  ReceiveSessionController::ReceiveSessionController(int &socket) {
    configureLogger("TTP2");
    this->socket = socket;
  }
  
  void ReceiveSessionController::receiveSession() {
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
          Packet::Packet packet = receiveMessage(fd);
          if (packet.id == -1) {
            continue;
          }
          pushResponse(packet);
        }
      }
    }
  }
  
  void ReceiveSessionController::disconnect() {
    std::lock_guard<std::mutex> lock(mtx);
    close(this->socket);
    connected = false;
  }
}
