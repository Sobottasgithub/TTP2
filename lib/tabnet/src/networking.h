#ifndef NETWORKING_H
#define NETWORKING_H

#include <string>
#include <vector>
#include <mutex>

class Networking
{
    public:
      struct Packet {
        int method;
        std::string payload;  
      };

      Networking();
      bool isConnected();
      bool hasOrder();
      bool hasSolution();
      Packet popOrder();
      Packet popSolution();
      int getOrderCollectionSize();
      int getSolutionCollectionSize();
      void pushSolution(Packet);
      void pushOrder(Packet order);
  
      int sendMessage(int socket, int method, std::string payload);
      int sendPacket(int socket, Packet packet);
      Packet receiveMessage(int socket);

    protected:
      bool connected = true;
      int socket;

      bool isNumeric(const std::string& string);
            
    private:
      std::vector<Packet> orderCollection;
      std::vector<Packet> solutionCollection;
      std::mutex mtx;
      Packet popCollection(std::vector<Packet> collection);
};

#endif
