#ifndef NETWORKING_H
#define NETWORKING_H

#include <string>
#include <vector>
#include <mutex>
#include <map>
#include <variant>

namespace ttp2 {
  class Networking
  {
      public:      
        struct Standard {
          std::string payload = "";
        };

        struct File {
        	std::string filePath = "";
          int start = -1;
        	int end = -1;
        	std::string payload = "";
        };

        struct Viewport {
          int xStart = 0;
          int xEnd = 0;
          int yStart = 0;
          int yEnd = 0;
          std::string payload = "";
        };
        
        typedef std::variant<Standard, File, Viewport> payloadVariants;
      
        struct Packet {
          int id = -1;
          payloadVariants payload;  
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
  
        int sendMessage(int socket, int id, payloadVariants payload);
        int sendPacket(int socket, Packet packet);
        Packet receiveMessage(int socket);

        static std::string getBroadcastIpAddress();
        static std::string getLocalIpAddress(std::string interface);
        static bool isValidIpV4(std::string &ipString);
      
      protected:      
        bool connected = true;
      
        bool isNumeric(const std::string& string);
        int bytesToInt(std::vector<char> bytes, int size);

        std::vector<Packet> requestQueue;
        std::vector<Packet> responseQueue;
        std::mutex mtx;
        ssize_t sendBytes(int socket, const char* buffer, size_t max);

        std::map<int, std::vector<unsigned char>> sessionBuffers;

        int autoId;
  };
}

#endif
