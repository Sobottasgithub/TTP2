#ifndef CLIENT_SESSION_CONTROLLER_H
#define CLIENT_SESSION_CONTROLLER_H

#include "../src/networking.h"

class ClientSessionController: public Networking
{
    public:
      ClientSessionController();
      ClientSessionController(int &socket);
      void networkingSession();

    private:
      int socket;

      void sendRequestSession();
      void receiveResponseSession();
      void validateConnection(int responseCode);
};

#endif
