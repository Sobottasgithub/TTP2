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
    while (true) {
      if (this->sendQueue.size() > 0) {
        AddressedPacket addressedPacket = this->sendQueue.at(0);
        const char *message = "HelloWorld!!!"; // TODO: Replace with real packet later!
        sendMessageTo(addressedPacket.ip, addressedPacket.port, message);
      }
    }
  }

  void PeerSessionController::sendPacket(AddressedPacket addressedPacket) {
    sendQueue.push_back(addressedPacket);
  }
  
  PeerSessionController::AddressedPacket PeerSessionController::receivePacket() {
    AddressedPacket addressedPacket;
    if (hasPacket()) {
      addressedPacket = this->receivedQueue.at(0);
      this->receivedQueue.erase(this->receivedQueue.begin());
      return addressedPacket;
    }
    return addressedPacket;
  }
  
  bool PeerSessionController::hasPacket() {
    if (this->receivedQueue.size() > 0) {
      return true;
    }
    return false;
  }
}
