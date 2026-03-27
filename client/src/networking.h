#ifndef NETWORKING_H
#define NETWORKING_H

#include "helpers.h"

#include <string>
#include <vector>
#include <mutex>

class Networking
{
    public:
      Networking();
      void networkingSession(int socket);
      bool isConnected();
      bool hasOrder();
      bool hasSolution();
      helpers::Packet popOrder();
      helpers::Packet popSolution();
      int getOrderCollectionSize();
      void pushSolution(helpers::Packet);
      void pushOrder(helpers::Packet order); 
      
    private:
      int socket;
      std::vector<helpers::Packet> orderCollection;
      std::vector<helpers::Packet> solutionCollection;
      bool connected = true;
      std::mutex mtx;
      helpers::Packet popCollection(std::vector<helpers::Packet> collection);
};

#endif
