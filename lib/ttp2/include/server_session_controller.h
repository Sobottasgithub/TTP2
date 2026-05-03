#ifndef SERVER_SESSION_CONTROLLER_H
#define SERVER_SESSION_CONTROLLER_H

#include "../src/networking.h"

class ServerSessionController: public Networking
{
    public:
      ServerSessionController();
      ServerSessionController(int serverSocket, int clientSocket);
      void networkingSession();
      std::string getLocalIpAddress(std::string interface);
    private:
      int serverSocket;
      int clientSocket;

      void sendResponseSession();
      void receiveRequestSession();
      void validateConnection(int responseCode);
};

#endif
