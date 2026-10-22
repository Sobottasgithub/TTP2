#ifndef UDP_NETWORKING_H
#define UDP_NETWORKING_H

#include <string>
#include <sys/epoll.h>
#include <tuple>
#include <tablog.h>

#include "../packet_types.h"

namespace ttp2 {
  class UdpNetworking {
    public:
      void createServerSocket(const char *ip, int port);
      void sendMessageTo(const char *ip, int port, const char *buffer, size_t size);
      std::tuple<std::string, int, Packet::Packet> receivePacketFrom();
      void sendPacketTo(const char *ip, int port, ttp2::Packet::Packet packet);

    protected:
      std::shared_ptr<tablog::Tablog> logger;

      int serverSocket;
      int epollFd;

      int autoId = 0;
  };
}

#endif
