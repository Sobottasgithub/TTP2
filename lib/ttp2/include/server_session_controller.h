#ifndef SERVER_SESSION_CONTROLLER_H
#define SERVER_SESSION_CONTROLLER_H

#include "networking.h"
#include <sys/epoll.h>

namespace ttp2 {
  class ServerSessionController: public Networking
  {
      public:
        ServerSessionController();
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
