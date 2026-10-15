#ifndef PEER_SESSION_CONTROLLER_H
#define PEER_SESSION_CONTROLLER_H

#include "core/udp_networking.h"

namespace ttp2 {
  class PeerSessionController : public UdpNetworking {
    public:
      PeerSessionController(const char *ip, int port);
      void networkingSession();
    private:
      void receiveSession();
      void sendSession();
  };
}

#endif
