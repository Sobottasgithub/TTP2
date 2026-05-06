#ifndef CLIENT_SESSION_CONTROLLER_H
#define CLIENT_SESSION_CONTROLLER_H

#include "../src/networking.h"
#include <sys/epoll.h>

class ClientSessionController: public Networking
{
    public:
      ClientSessionController();
      ClientSessionController(int &socket);
      void networkingSession();

    private:
      int socket;

      int epollFd;
      struct epoll_event serverEvent;

      void sendRequestSession();
      void receiveResponseSession();
};

#endif
