#ifndef Client_SESSION_CONTROLLER_H
#define Client_SESSION_CONTROLLER_H

#include "../src/networking.h"

class ClientSessionController: public Networking
{
    public:
      void networkingSession(int serverSocket, int clientSocket);
};

#endif
