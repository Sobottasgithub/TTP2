#ifndef NETWORKING_H
#define NETWORKING_H

#include "packet_types.h"

#include <tablog.h>

#include <string>
#include <vector>
#include <mutex>
#include <map>
#include <variant>
#include <memory>
#include <arrow/api.h>
#include <arrow/ipc/api.h>
#include <arrow/io/api.h>

namespace ttp2 {
  class Networking
  {
      public:
        bool isConnected();
        bool hasRequest();
        bool hasResponse();
        Packet::Packet popRequest();
        Packet::Packet popResponse();
        int getRequestQueueSize();
        int getResponseQueueSize();
        void pushResponse(Packet::Packet);
        void pushRequest(Packet::Packet request);
  
        int sendMessage(int socket, int id, Packet::payloadVariants payload);
        int sendPacket(int socket, Packet::Packet packet);
        Packet::Packet receiveMessage(int socket);

        // WARNING: This struct cant be send as a payload type!
        struct PacketInfo {
          int id;
          Packet::payloadVariants payloadType;
        };
        PacketInfo peekResponse();
        PacketInfo peekResponse(int index);
        PacketInfo peekRequest();
        PacketInfo peekRequest(int index);

        static std::string getBroadcastIpAddress();
        static std::string getLocalIpAddress(std::string interface);
        static bool isValidIpV4(std::string &ipString);
        static bool isValidInterface(std::string &interface);

        virtual void disconnect();
        
      protected:
        std::shared_ptr<tablog::Tablog> logger;
        
        bool connected = true;
      
        bool isNumeric(const std::string& string);
        int bytesToInt(std::vector<char> bytes, int size);

        std::shared_ptr<arrow::Buffer> tableToBuffer(const std::shared_ptr<arrow::Table>& table);
        std::shared_ptr<arrow::Table> bufferToTable(const uint8_t* rawData, int64_t dataSize);

        void configureLogger(std::string name);

        std::vector<Packet::Packet> requestQueue;
        std::vector<Packet::Packet> responseQueue;
        std::mutex mtx;
        ssize_t sendBytes(int socket, const char* buffer, size_t max);

        std::map<int, std::vector<unsigned char>> sessionBuffers;

        int autoId = 0;
  };
}

#endif
