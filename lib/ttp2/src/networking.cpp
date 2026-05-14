#include "../include/networking.h"

#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <mutex>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>

extern "C" {
#include <libtasn1.h>
extern const asn1_static_node packets_asn1_tab[];
}

bool Networking::hasResponse() {
  std::lock_guard<std::mutex> lock(mtx);
  return !responseQueue.empty();
}

bool Networking::hasRequest() {
  std::lock_guard<std::mutex> lock(mtx);
  return !requestQueue.empty();
}

bool Networking::isConnected() {
  std::lock_guard<std::mutex> lock(mtx);
  return connected;
}

void Networking::disconnect() {
  std::lock_guard<std::mutex> lock(mtx);
  connected = false;
}

Networking::Packet Networking::popRequest() {
  std::lock_guard<std::mutex> lock(mtx);
  if (!requestQueue.empty()) {
    Networking::Packet firstRequest = requestQueue[0];
    requestQueue.erase(requestQueue.begin());
    return firstRequest;
  }
  Networking::Packet emptyPacket;
  return emptyPacket;
}

Networking::Packet Networking::popResponse() {
  std::lock_guard<std::mutex> lock(mtx);
  if (!responseQueue.empty()) {
    Networking::Packet firstResponse = responseQueue[0];
    responseQueue.erase(responseQueue.begin());
    return firstResponse;
  }
  Networking::Packet emptyPacket;
  return emptyPacket;
}

void Networking::pushResponse(Networking::Packet response) {
  std::lock_guard<std::mutex> lock(mtx);
  responseQueue.push_back(response);
}

void Networking::pushRequest(Networking::Packet request) {
  std::lock_guard<std::mutex> lock(mtx);
  requestQueue.push_back(request);
}

int Networking::getRequestQueueSize() {
  std::lock_guard<std::mutex> lock(mtx);
  return requestQueue.size();
}

int Networking::getResponseQueueSize() {
  std::lock_guard<std::mutex> lock(mtx);
  return responseQueue.size();
}

int Networking::sendPacket(int socket, Networking::Packet packet) {
  return sendMessage(socket, packet.id, packet.method, packet.payload);
}

int Networking::sendMessage(int socket, int id, int method,
                            std::string payload) {
  asn1_node definitions = nullptr;
  asn1_node packet = nullptr;
  char errorDescription[ASN1_MAX_ERROR_DESCRIPTION_SIZE];

  if (asn1_array2tree(packets_asn1_tab, &definitions, errorDescription) !=
      ASN1_SUCCESS) {
    std::wcout << "Error in sendMessage when loading asn1:  "
               << errorDescription << std::endl;
    return -1;
  }

  asn1_create_element(definitions, "Packets.Packet", &packet);

  std::string idString = std::to_string(id);
  asn1_write_value(packet, "id", idString.c_str(), 0);

  std::string methodString = std::to_string(method);
  asn1_write_value(packet, "method", methodString.c_str(), 0);

  const char *messageChar = payload.c_str();
  asn1_write_value(packet, "payload", messageChar, strlen(messageChar));

  int derLen = 0;
  asn1_der_coding(packet, "", nullptr, &derLen, nullptr);
  std::vector<unsigned char> buffer(derLen);
  if (asn1_der_coding(packet, "", buffer.data(), &derLen, errorDescription) !=
      ASN1_SUCCESS) {
    std::wcout << "Error while encoding packet: " << errorDescription
               << std::endl;
    return -1;
  }

  uint32_t size = htonl(derLen);
  sendBytes(socket, reinterpret_cast<char *>(&size), sizeof(size));
  sendBytes(socket, reinterpret_cast<char *>(buffer.data()), derLen);

  asn1_delete_structure(&packet);
  asn1_delete_structure(&definitions);

  return 0;
}

Networking::Packet Networking::receiveMessage(int socket) {
  Networking::Packet data = {-1, -1, ""};
  unsigned char temp[4096];
  while (true) {
    ssize_t n = recv(socket, temp, sizeof(temp), 0);
    if (n > 0) {
      sessionBuffers[socket].insert(sessionBuffers[socket].end(), temp,
                                    temp + n);
    } else if (n < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        break;
      }
      // std::wcout << "Error while receiving!" << std::endl;
      return data;
    } else {
      // std::wcout << "Socket closed!" << std::endl;
      return data;
    }
  }

  auto &buffer = sessionBuffers[socket];

  if (buffer.size() < sizeof(uint32_t)) {
    return data;
  }

  uint32_t networkSize;
  memcpy(&networkSize, buffer.data(), sizeof(uint32_t));
  uint32_t derLen = ntohl(networkSize);

  if (buffer.size() < sizeof(uint32_t) + derLen) {
    return data;
  }

  std::vector<char> derBuffer(buffer.begin() + sizeof(uint32_t),
                              buffer.begin() + sizeof(uint32_t) + derLen);

  buffer.erase(buffer.begin(), buffer.begin() + sizeof(uint32_t) + derLen);

  asn1_node definitions = nullptr;
  asn1_node packet = nullptr;
  char errorDescription[ASN1_MAX_ERROR_DESCRIPTION_SIZE];

  if (asn1_array2tree(packets_asn1_tab, &definitions, errorDescription) !=
      ASN1_SUCCESS) {
    return data;
  }

  asn1_create_element(definitions, "Packets.Packet", &packet);

  if (asn1_der_decoding(&packet, derBuffer.data(), derLen, errorDescription) ==
      ASN1_SUCCESS) {
    unsigned char idBin[8];
    int idLen = sizeof(idBin);
    if (asn1_read_value(packet, "id", idBin, &idLen) == ASN1_SUCCESS) {
      long idValue = 0;
      for (int i = 0; i < idLen; i++) {
        idValue = (idValue << 8) | idBin[i];
      }
      data.id = static_cast<int>(idValue);
    }

    unsigned char methodBin[8];
    int methodLen = sizeof(methodBin);
    if (asn1_read_value(packet, "method", methodBin, &methodLen) ==
        ASN1_SUCCESS) {
      long methodValue = 0;
      for (int i = 0; i < methodLen; i++) {
        methodValue = (methodValue << 8) | methodBin[i];
      }
      data.method = static_cast<int>(methodValue);
    }

    int payloadLen = 0;
    asn1_read_value(packet, "payload", nullptr, &payloadLen);
    if (payloadLen > 0) {
      std::vector<char> payloadStr(payloadLen);
      asn1_read_value(packet, "payload", payloadStr.data(), &payloadLen);
      data.payload.assign(payloadStr.data(), payloadLen);
    }
  } else {
    std::wcout << "Error decoding ASN1" << std::endl;
  }

  asn1_delete_structure(&packet);
  asn1_delete_structure(&definitions);

  return data;
}

ssize_t Networking::sendBytes(int socket, const char *buffer, size_t max) {
  size_t sentBytes = 0;
  while (sentBytes < max) {
    ssize_t sent = send(socket, buffer + sentBytes, max - sentBytes, 0);

    if (sent <= 0)
      return -1;

    sentBytes += sent;
  }
  return sentBytes;
}
