#ifndef UDP_NETWORKING_H
#define UDP_NETWORKING_H

#include <string>
#include <sys/epoll.h>

namespace ttp2 {
  class UdpNetworking {
    public:
      void createServerSocket(const char *ip, int port);
      void sendMessageTo(const char *ip, int port, const char *buffer);
      std::string receivePacketFrom();

    protected:      
      int serverSocket;
      int epollFd;
  };
}

#endif
