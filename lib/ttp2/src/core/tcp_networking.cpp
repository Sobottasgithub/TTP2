#include "../../include/core/tcp_networking.h"
#include "packet_types.h"
#include "../include/helpers/asn1_decode.h"
#include "helpers/asn1_encode.h"

#include <tablog_registry.h>
#include <tablog.h>
#include <tabcrypt.h>

#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <ifaddrs.h>
#include <memory>
#include <mutex>
#include <net/if.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <arrow/api.h>
#include <arrow/ipc/api.h>
#include <arrow/io/api.h>
#include <arrow/csv/api.h>
#include <arrow/buffer.h>
#include <arrow/io/memory.h>
#include <arrow/ipc/reader.h>
#include <arrow/ipc/writer.h>
#include <arrow/result.h>
#include <arrow/status.h>
#include <arrow/table.h>


extern "C" {
#include <libtasn1.h>
extern const asn1_static_node packets_asn1_tab[];
}

namespace ttp2 {
  bool TcpNetworking::hasResponse() {
    std::lock_guard<std::mutex> lock(responseMutex);
    return !responseQueue.empty();
  }

  bool TcpNetworking::hasRequest() {
    std::lock_guard<std::mutex> lock(requestMutex);
    return !requestQueue.empty();
  }

  bool TcpNetworking::isConnected() {
    std::lock_guard<std::mutex> lock(connectedMutex);
    return connected;
  }

  Packet::Packet TcpNetworking::popRequest() {
    std::unique_lock<std::mutex> lock(requestMutex);
    requestConditionVariable.wait(lock, [this] { return !requestQueue.empty() || !connected; });

    if (!connected) {
      Packet::Packet emptyPacket;
      return emptyPacket;
    }
    
    Packet::Packet firstRequest = requestQueue[0];
    requestQueue.erase(requestQueue.begin());
    return firstRequest;
  }

  Packet::Packet TcpNetworking::popResponse() {
    std::unique_lock<std::mutex> lock(responseMutex);
    responseConditionVariable.wait(lock, [this] { return !responseQueue.empty() || !connected; });

    if (!connected) {
      Packet::Packet emptyPacket;
      return emptyPacket;
    }
    
    Packet::Packet firstResponse = responseQueue[0];
    responseQueue.erase(responseQueue.begin());
    return firstResponse;
  }

  void TcpNetworking::pushResponse(Packet::Packet response) {
    {
      std::lock_guard<std::mutex> lock(responseMutex);
      responseQueue.emplace_back(response);
    }
    responseConditionVariable.notify_one();
  }

  void TcpNetworking::pushRequest(Packet::Packet request) {
    {
      std::lock_guard<std::mutex> lock(requestMutex);
      requestQueue.emplace_back(request);
    }
    requestConditionVariable.notify_one();
  }

  int TcpNetworking::getRequestQueueSize() {
    std::lock_guard<std::mutex> lock(requestMutex);
    return requestQueue.size();
  }

  int TcpNetworking::getResponseQueueSize() {
    std::lock_guard<std::mutex> lock(responseMutex);
    return responseQueue.size();
  }

  int TcpNetworking::sendPacket(int socket, Packet::Packet packet) {
    return sendMessage(socket, packet.id, packet.payload);
  }

  int TcpNetworking::sendMessage(int socket, int id,
                              Packet::payloadVariants payload) {
    if (id == -1) {
      id = autoId;
      autoId++;
    }

    std::vector<unsigned char> buffer;
    try {
      buffer = asn1::encode::Encode::encode(payload, id);
    } catch (const std::invalid_argument& invalidArgument) {
      logger->log(tablog::ERROR, invalidArgument.what());
      return -1;
    }
    
    if (buffer.size() > UINT32_MAX) {
      logger->log(tablog::ERROR, "Payload too large to send. The max is 4gb");
      return -1;
    }

    if (this->useEncryption) {
      buffer = tabcrypt::encrypt(this->encryptSecret, buffer);
    }

    uint32_t netSize = htonl(static_cast<uint32_t>(buffer.size()));
    
    if (sendBytes(socket, reinterpret_cast<const char *>(&netSize), sizeof(netSize)) != sizeof(netSize)) {
      return -1;
    }

    if (sendBytes(socket, reinterpret_cast<const char *>(buffer.data()), buffer.size()) != static_cast<ssize_t>(buffer.size())) {
      return -1;
    }

    return 0;
  }

  ssize_t TcpNetworking::sendBytes(int socket, const char *buffer, size_t max) {
    size_t sentBytes = 0;
    while (sentBytes < max) {
      ssize_t sent = send(socket, buffer + sentBytes, max - sentBytes, MSG_NOSIGNAL);

      if (sent < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          continue; 
        }
        logger->log(tablog::ERROR, "Send failed with errno: " + std::string(strerror(errno)));
        return -1;
      }

      if (sent == 0) {
        return -1;
      }

      sentBytes += sent;
    }
    return sentBytes;
  }


  Packet::Packet TcpNetworking::receiveMessage(int socket) {
    Packet::Packet data;

    unsigned char temp[4096];
    while (isConnected()) {
      ssize_t n = recv(socket, temp, sizeof(temp), 0);
      if (n > 0) {
        sessionBuffers[socket].insert(sessionBuffers[socket].end(), temp, temp + n);
      } else if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
          break;
        }
        logger->log(tablog::ERROR, "Error while receiving bytes: " + std::string(strerror(errno)));
        disconnect();
        return data;
      } else {
        disconnect();
        return data;
      }
    }

    auto &buffer = sessionBuffers[socket];

    if (this->useDecryption) {
      buffer = tabcrypt::decrypt(this->decryptSecret, buffer);
    }

    if (buffer.size() < sizeof(uint32_t)) {
      return data;
    }

    uint32_t networkSize;
    std::memcpy(&networkSize, buffer.data(), sizeof(uint32_t));
    uint32_t derLen = ntohl(networkSize);

    size_t totalMessageSize = sizeof(uint32_t) + derLen;
    if (buffer.size() < totalMessageSize) {
      return data;
    }

    std::vector<char> derBuffer(
        buffer.begin() + sizeof(uint32_t),
        buffer.begin() + totalMessageSize
    );

    buffer.erase(buffer.begin(), buffer.begin() + totalMessageSize);
    
    try {
      return asn1::decode::Decode::decode(derBuffer);
    } catch (const std::invalid_argument& invalidArgument) {
      logger->log(tablog::ERROR, invalidArgument.what());
      return data;
    }
  }

  TcpNetworking::PacketInfo TcpNetworking::peekResponse() {
    return peekResponse(0);
  }

  TcpNetworking::PacketInfo TcpNetworking::peekResponse(int index) {
    TcpNetworking::PacketInfo packetInfo;
    if (index > getResponseQueueSize() - 1 || index < 0) {
      logger->log(tablog::ERROR, "Invalid peek request: " + std::to_string(index));
      return packetInfo;
    }

    std::lock_guard<std::mutex> lock(responseMutex);
    packetInfo.id = responseQueue[index].id;
    packetInfo.payloadType = responseQueue[index].payload;
    return packetInfo;
  }

  TcpNetworking::PacketInfo TcpNetworking::peekRequest() {
    return peekRequest(0);
  }

  TcpNetworking::PacketInfo TcpNetworking::peekRequest(int index) {
    TcpNetworking::PacketInfo packetInfo;
    if (index > getRequestQueueSize() - 1 || index < 0) {
      logger->log(tablog::ERROR, "Invalid peek request: " + std::to_string(index));
      return packetInfo;
    }

    std::lock_guard<std::mutex> lock(requestMutex);
    packetInfo.id = requestQueue[index].id;
    packetInfo.payloadType = requestQueue[index].payload;
    return packetInfo;
  }

  int TcpNetworking::bytesToInt(std::vector<char> bytes, int size) {
    int result = 0;
    for (int index = 0; index < size; index++)
    {
        result <<= 8;
        result |= (bytes[index] & 0xFF);
    }
    return result;
  }

  void TcpNetworking::disconnect() {}

  void TcpNetworking::configureLogger(std::string name) {
    tablog::TablogRegistry* registry = &tablog::TablogRegistry::getInstance();
    std::shared_ptr<tablog::Tablog> logger = std::make_shared<tablog::Tablog>();
    logger->configure(name, true);
    registry->registerLogger(name, logger);
    this->logger = logger;
  }
}
