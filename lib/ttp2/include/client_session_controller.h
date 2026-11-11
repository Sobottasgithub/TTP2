#ifndef CLIENT_SESSION_CONTROLLER_H
#define CLIENT_SESSION_CONTROLLER_H

#include "core/tcp_networking.h"
#include <sys/epoll.h>

namespace ttp2 {
  class ClientSessionController: public TcpNetworking
  {
      public:
        ClientSessionController(std::string ipV4, int port);
        ClientSessionController(int &socket);
        void networkingSession();
        void disconnect() override;
        bool isInitCompleat();

      private:
        int socket;

        int epollFd;
        struct epoll_event serverEvent;

        std::mutex initCompleatMutex;
        bool initCompleat = false;

        void sendRequestSession();
        void receiveResponseSession();
  };
}

#endif
