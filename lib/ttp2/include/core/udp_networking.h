#ifndef UDP_NETWORKING_H
#define UDP_NETWORKING_H

namespace ttp2 {
  class UdpNetworking {
    public:
      void createServerSocket(const char *ip, int port);
      void sendMessageTo(const char *ip, int port, const char *buffer);

    protected:
      int serverSocket;
      int epollFd;
  };
}

#endif
