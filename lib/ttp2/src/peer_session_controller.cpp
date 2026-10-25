#include "../include/peer_session_controller.h"

#include <mutex>
#include <thread>
#include <sys/epoll.h>
#include <unistd.h>

#include <tablog.h>
#include <tablog_registry.h>

namespace ttp2 {
  PeerSessionController::PeerSessionController(const char *ip, int port) {
    std::string name = "Peer - TTP2";
    tablog::TablogRegistry* registry = &tablog::TablogRegistry::getInstance();
    this->logger = std::make_shared<tablog::Tablog>();

    struct LoglevelConfig {
      std::string color = "";
      bool visible = true;
    };
    tablog::Tablog::LoglevelConfig debugConfig;
    debugConfig.visible = false;
    this->logger->configure(name, true, true, true, false, debugConfig);
    registry->registerLogger(name, logger);

    this->createServerSocket(ip, port);
  }
  
  void PeerSessionController::networkingSession() {
    std::thread sendSessionThread([this]() {
        this->sendSession();
    });

    std::thread receiveSessionThread([this]() {
        this->receiveSession();
    });
 
    while (isConnected()) {
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

    while (isConnected()) {
      int receivedEvents = epoll_wait(this->epollFd, events, maxEvents, 1000);
      if (receivedEvents == -1) {
        if (errno == EINTR) continue;
      }

      for (int index = 0; index < receivedEvents; ++index) {
        if (events[index].data.fd == this->serverSocket) {
          AddressedPacket addressedPacket;
          std::unique_lock<std::mutex> lock(receiveMutex);
          // type INFO: string, int Packet::Packet
          auto [ip, port, packet] = receivePacketFrom();
          addressedPacket.ip = ip.c_str();
          addressedPacket.port = port;
          addressedPacket.packet = packet;
          logger->log(tablog::DEBUG, "Received message from ip: " + ip);
          receivedQueue.push_back(addressedPacket);
          lock.unlock();
        }
      }
    }
  }
  
  void PeerSessionController::sendSession() {
    while (isConnected()) {
      std::unique_lock<std::mutex> lock(sendMutex);
      if (this->sendQueue.size() > 0) {
        AddressedPacket addressedPacket = this->sendQueue.at(0);
        this->sendQueue.erase(this->sendQueue.begin());
        sendPacketTo(addressedPacket.ip, addressedPacket.port, addressedPacket.packet);
        logger->log(tablog::DEBUG, "Packet sent");
      }
      lock.unlock();
    }
  }

  void PeerSessionController::sendPacket(AddressedPacket addressedPacket) {
    std::lock_guard<std::mutex> lock(sendMutex);
    sendQueue.push_back(addressedPacket);
  }
  
  PeerSessionController::AddressedPacket PeerSessionController::popReceivedPacket() {
    std::lock_guard<std::mutex> lock(receiveMutex);
    AddressedPacket addressedPacket;
    if (receivedQueue.size() > 0) {
      addressedPacket = this->receivedQueue.at(0);
      this->receivedQueue.erase(this->receivedQueue.begin());
      return addressedPacket;
    }
    return addressedPacket;
  }
  
  bool PeerSessionController::hasPacket() {
    std::lock_guard<std::mutex> lock(receiveMutex);
    if (this->receivedQueue.size() > 0) {
      return true;
    }
    return false;
  }

  bool PeerSessionController::isConnected() {
    std::lock_guard<std::mutex> lock(connectedMutex);
    return this->connected;
  }
  
  void PeerSessionController::disconnect() {
    std::lock_guard<std::mutex> lock(connectedMutex);
    logger->log(tablog::DEBUG, "Disconnect");
    this->connected = false;
    close(this->serverSocket);
    close(this->epollFd);
  }
}
