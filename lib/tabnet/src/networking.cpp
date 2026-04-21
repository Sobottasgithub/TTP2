#include "networking.h"

#include "../include/methods.h"

#include <mutex>
#include <netinet/in.h>
#include <string>
#include <iostream>
#include <regex>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>

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
    return sendMessage(socket, packet.method, packet.payload);
}

int Networking::sendMessage(int socket, int method, std::string payload) {
  asn1_node definitions = nullptr;
  asn1_node packet = nullptr;
  char errorDescription[ASN1_MAX_ERROR_DESCRIPTION_SIZE];

  // Load asn1 definition
  if (asn1_array2tree(packets_asn1_tab, &definitions, errorDescription) != ASN1_SUCCESS) {
      std::wcout << "Error in sendMessage when loading asn1:  " << errorDescription << std::endl;
      return -1;
  }

  asn1_create_element(definitions, "Packets.Packet", &packet);

  // Integers must be written as strings in asn1
  std::string methodString = std::to_string(method); 
  asn1_write_value(packet, "method", methodString.c_str(), 0);

  // Octet Strings must be written as raw bytes
  const char* messageChar = payload.c_str();
  asn1_write_value(packet, "payload", messageChar, strlen(messageChar));

  // Encode
  int derLen = 0;
  // Get future len of encoded packet
  asn1_der_coding(packet, "", nullptr, &derLen, nullptr);
  std::vector<unsigned char> buffer(derLen);
  // Write encoded packet to buffer
  if (asn1_der_coding(packet, "", buffer.data(), &derLen, errorDescription) != ASN1_SUCCESS) {
      std::wcout << "Error while encoding packet: " << errorDescription << std::endl;
      return -1;
  }

  uint32_t size = htonl(derLen); // Convert to bytes
  sendBytes(socket, reinterpret_cast<char*>(&size), sizeof(size)); // [1] Send size (4 bytes)
  sendBytes(socket, reinterpret_cast<char*>(buffer.data()), derLen); // [2] Send data

  asn1_delete_structure(&packet);
  asn1_delete_structure(&definitions);
  
  return 0;
}

Networking::Packet Networking::receiveMessage(int socket) {
  Networking::Packet data;
  asn1_node definitions = nullptr;
  asn1_node packet = nullptr;
  char errorDescription[ASN1_MAX_ERROR_DESCRIPTION_SIZE];

  // Load asn1 definition
  if (asn1_array2tree(packets_asn1_tab, &definitions, errorDescription) != ASN1_SUCCESS) {
      std::wcout << "Error in sendMessage when loading asn1:  " << errorDescription << std::endl;
      return data;
  }

  asn1_create_element(definitions, "Packets.Packet", &packet);

  uint32_t size;  
  receiveBytes(socket, reinterpret_cast<unsigned char*>(&size), sizeof(size)); // [1] Receive size
  int derLen = ntohl(size);
  // [2] Receive data
  // std::vector<char> buffer(size);
  // receiveBytes(socket, buffer.data(), size);
  std::vector<char> buffer(size);
  recv(socket, buffer.data(), size, 0);

  if (asn1_der_decoding(&packet, buffer.data(), derLen, errorDescription) != ASN1_SUCCESS) {
    std::cerr << "Decode error: " << errorDescription << std::endl;
    return data;
  }

  unsigned char methodBin[8];
  int methodLen = sizeof(methodBin);
  asn1_read_value(packet, "method", methodBin, &methodLen);

  // Convert raw byte to a int
  long methodVal = 0;
  for (int i = 0; i < methodLen; i++) {
      methodVal = (methodVal << 8) | methodBin[i];
  }
  int method = methodVal;
  
  // Read payload
  std::vector<char> payloadStr(128);
  int payloadLen = payloadStr.size();
  asn1_read_value(packet, "payload", payloadStr.data(), &payloadLen);
  std::string payload = std::string(payloadStr.data(), payloadLen);

  asn1_delete_structure(&packet);
  asn1_delete_structure(&definitions);


  data = {method, payload};
  return data;
}

bool Networking::isNumeric(const std::string& string) {
  static const std::regex numberRegex(
      R"(^[-+]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][-+]?\d+)?$)"
  );
  return std::regex_match(string, numberRegex);
}

ssize_t Networking::receiveBytes(int socket, unsigned char* buffer, size_t max) {
    size_t receivedBytes = 0;
    while (receivedBytes < max) {
        ssize_t received = recv(socket, buffer + receivedBytes, max - receivedBytes, 0);
        
        if (received == 0) return 0;
        if (received < 0) return -1;
        
        receivedBytes += received;
    }
    return receivedBytes;
}

ssize_t Networking::sendBytes(int socket, const char* buffer, size_t max) {
    size_t sentBytes = 0;
    while (sentBytes < max) {
        ssize_t sent = send(socket, buffer + sentBytes, max - sentBytes, 0);
        
        if (sent <= 0) return -1;
        
        sentBytes += sent;
    }
    return sentBytes;
}
