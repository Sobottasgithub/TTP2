#ifndef SERVER_SESSION_CONTROLLER_H
#define SERVER_SESSION_CONTROLLER_H

#include "core/tcp_networking.h"
#include <sys/epoll.h>

namespace ttp2 {
  class ServerSessionController: public TcpNetworking
  {
      public:
        ServerSessionController(int serverSocket, int clientSocket);
        void networkingSession();
        void disconnect() override;
        
      private:
        int serverSocket;
        int clientSocket;

        int epollFd;
        struct epoll_event clientEvent;

        void sendResponseSession();
        void receiveRequestSession();
  };
}

#endif
