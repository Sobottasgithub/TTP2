#ifndef CLIENT_SESSION_CONTROLLER_H
#define CLIENT_SESSION_CONTROLLER_H

#include "../src/networking.h"

class ClientSessionController: public Networking
{
    public:
      void networkingSession(int socket);
      void testAsn1();
};

#endif
