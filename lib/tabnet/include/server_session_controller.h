#ifndef SERVER_SESSION_CONTROLLER_H
#define SERVER_SESSION_CONTROLLER_H

#include "../src/networking.h"

class ServerSessionController: public Networking
{
    public:
      void networkingSession(int serverSocket, int clientSocket);
};

#endif
