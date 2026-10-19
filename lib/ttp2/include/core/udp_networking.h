#ifndef UDP_NETWORKING_H
#define UDP_NETWORKING_H

#include <string>
#include <sys/epoll.h>

#include "../packet_types.h"

namespace ttp2 {
  class UdpNetworking {
    public:
      void createServerSocket(const char *ip, int port);
      void sendMessageTo(const char *ip, int port, const char *buffer);
      Packet::Packet receivePacketFrom();
      void sendPacketTo(const char *ip, int port, ttp2::Packet::Packet packet);

    protected:      
      int serverSocket;
      int epollFd;
  };
}

#endif
