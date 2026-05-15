#ifndef NETWORKING_H
#define NETWORKING_H

#include <string>
#include <vector>
#include <mutex>
#include <map>
#include <variant>

class Networking
{
    public:
      struct Standard {
        std::string payload = "";
      };
      
      struct Packet {
        int id = -1;
        std::variant<Standard> payload;  
      };

      bool isConnected();
      void disconnect();
      bool hasRequest();
      bool hasResponse();
      Packet popRequest();
      Packet popResponse();
      int getRequestQueueSize();
      int getResponseQueueSize();
      void pushResponse(Packet);
      void pushRequest(Packet request);
  
      int sendMessage(int socket, int id, std::variant<Standard> payload);
      int sendPacket(int socket, Packet packet);
      Packet receiveMessage(int socket);

    protected:
      bool connected = true;
      
      bool isNumeric(const std::string& string);
            
      std::vector<Packet> requestQueue;
      std::vector<Packet> responseQueue;
      std::mutex mtx;
      ssize_t sendBytes(int socket, const char* buffer, size_t max);

      std::map<int, std::vector<unsigned char>> sessionBuffers;
};

#endif
