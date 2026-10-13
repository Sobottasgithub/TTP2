#ifndef PEER_SESSION_CONTROLLER_H
#define PEER_SESSION_CONTROLLER_H

#include "send_session_controller.h"
#include "receive_session_controller.h"
#include "packet_types.h"

#include <string>
#include <memory>

namespace ttp2 {
  class PeerSessionController {
    public:
      PeerSessionController(std::string interface, int port);

      void connectTo(std::string ip);
      void connectTo(std::string ip, int port);
      std::vector<Packet> getReceivedPacketsFromIp(std::string ip);
      void sendPacketToIp(std::string ip);

      void serverAccpetCycle();

    private:
      // TODO: mutex
      int serverSocket;
      int epollFd;
      int port;
      
      struct Connection {
        std::string ip;
        std::shared_ptr<SendSessionController> sendSessionController;
        std::shared_ptr<ReceiveSessionController> receiveSessionController;
      };
      std::vector<Connection> peerConnections;

      void sendManager(int clientSocket);
      void receiveManager();
  };
}

#endif
