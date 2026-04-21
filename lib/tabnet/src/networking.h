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
      
      bool isNumeric(const std::string& string);
            
      std::vector<Packet> orderCollection;
      std::vector<Packet> solutionCollection;
      std::mutex mtx;
      ssize_t receiveBytes(int socket, unsigned char* buffer, size_t max);
      ssize_t sendBytes(int socket, const char* buffer, size_t max);
};

#endif
