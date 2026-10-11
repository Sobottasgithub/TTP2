#ifndef RECEIVE_SESSION_CONTROLLER_H
#define RECEIVE_SESSION_CONTROLLER_H

#include "networking.h"

#include <sys/epoll.h>

namespace ttp2 {
  class ReceiveSessionController: public Networking {
    public:
      ReceiveSessionController();
      void receiveSession();
      void disconnect() override;

    private:
      int socket;
      int epollFd;
      struct epoll_event serverEvent;
  };
}

#endif
