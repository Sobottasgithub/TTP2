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
      bool isConnected();
      bool hasOrder();
      bool hasSolution();
      helpers::Packet popOrder();
      helpers::Packet popSolution();
      int getOrderCollectionSize();
      int getSolutionCollectionSize();
      void pushSolution(helpers::Packet);
      void pushOrder(helpers::Packet order);

    protected:
      bool connected = true;
      int socket;
            
    private:
      std::vector<helpers::Packet> orderCollection;
      std::vector<helpers::Packet> solutionCollection;
      std::mutex mtx;
      helpers::Packet popCollection(std::vector<helpers::Packet> collection);
};

#endif
