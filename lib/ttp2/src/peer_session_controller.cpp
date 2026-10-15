#include "../include/peer_session_controller.h"

#include <thread>
#include <sys/epoll.h>

namespace ttp2 {
  PeerSessionController::PeerSessionController(const char *ip, int port) {
    this->createServerSocket(ip, port);
  }
  
  void PeerSessionController::networkingSession() {
    std::thread sendSessionThread([this]() {
        this->sendSession();
    });

    std::thread receiveSessionThread([this]() {
        this->receiveSession();
    });
 
    while (true) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    if (sendSessionThread.joinable()) {
      sendSessionThread.join();
    }

    if (receiveSessionThread.joinable()) {
      receiveSessionThread.join();
    }
  }
  
  void PeerSessionController::receiveSession() {
    static const int maxEvents = 100;
    epoll_event events[maxEvents];

    while (true) {
      int receivedEvents = epoll_wait(this->epollFd, events, maxEvents, -1);
      if (receivedEvents == -1) {
        if (errno == EINTR) continue;
      }

      for (int index = 0; index < receivedEvents; ++index) {
        if (events[index].data.fd == this->serverSocket) {
          receivePacketFrom(); // TODO: Push received Packet
        }
      }
    }
  }
  
  void PeerSessionController::sendSession() {
    // TODO: send all Packets from queue
  } 
}
