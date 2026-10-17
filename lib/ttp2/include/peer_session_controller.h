#ifndef PEER_SESSION_CONTROLLER_H
#define PEER_SESSION_CONTROLLER_H

#include "core/udp_networking.h"
#include "packet_types.h"

#include <vector>
#include <mutex>

namespace ttp2 {
  class PeerSessionController : public UdpNetworking {
    public:
      struct AddressedPacket {
        const char *ip;
        int port = 0;
        Packet::Packet packet;
      };
      
      PeerSessionController(const char *ip, int port);
      void networkingSession();

      void sendPacket(AddressedPacket addressedPacket);
      AddressedPacket receivePacket();
      bool hasPacket();

    private:
      std::mutex receiveMutex;
      std::mutex sendMutex;
      
      std::vector<AddressedPacket> receivedQueue = {};
      std::vector<AddressedPacket> sendQueue = {};
      
      void receiveSession();
      void sendSession();
  };
}

#endif
