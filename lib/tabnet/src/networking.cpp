#include "networking.h"

#include "../include/methods.h"

#include <mutex>
#include <string>
#include <map>
#include <iostream>
#include <regex>
#include <cstring>
#include <sys/socket.h>
#include <thread>

extern "C" {
#include <libtasn1.h>
extern const asn1_static_node packets_asn1_tab[];
}

Networking::Networking() {}

bool Networking::hasSolution() {
  std::lock_guard<std::mutex> lock(mtx);
  return !solutionCollection.empty();
}

bool Networking::hasOrder() {
  std::lock_guard<std::mutex> lock(mtx);
  return !orderCollection.empty();
}

bool Networking::isConnected() {
  std::lock_guard<std::mutex> lock(mtx);
  return connected;
}

Networking::Packet Networking::popOrder() {
  return popCollection(orderCollection);
}

Networking::Packet Networking::popSolution() {
  return popCollection(solutionCollection);
}

Networking::Packet Networking::popCollection(std::vector<Networking::Packet> collection) {
  std::lock_guard<std::mutex> lock(mtx);
  if (!collection.empty()) {
    Networking::Packet firstOrder = collection[0];
    collection.erase(collection.begin());  
    return firstOrder;
  }
  Networking::Packet emptyPacket;
  return emptyPacket;
}

void Networking::pushSolution(Networking::Packet solution) {
  std::lock_guard<std::mutex> lock(mtx);
  solutionCollection.push_back(solution);
}

void Networking::pushOrder(Networking::Packet order) {
  std::lock_guard<std::mutex> lock(mtx);
  orderCollection.push_back(order);
}

int Networking::getOrderCollectionSize() {
  std::lock_guard<std::mutex> lock(mtx);
  return orderCollection.size();
}

int Networking::getSolutionCollectionSize() {
  std::lock_guard<std::mutex> lock(mtx);
  return solutionCollection.size();
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

  std::wcout << "Encoded to " << derLen << " bytes." << std::endl;
  // TODO: Send message!

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

  // TODO: Receive message and write into buffer below:
  //std::vector<unsigned char> buffer(derLen);
  std::vector<unsigned char> buffer(128); // WARNING: temp only!
  int derLen = buffer.size(); // WARNING: temp only!
  
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
