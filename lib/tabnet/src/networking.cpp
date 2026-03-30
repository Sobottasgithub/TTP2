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
  return 0;
}

Networking::Packet Networking::receiveMessage(int socket) {
    Networking::Packet data;      
    return data;
}

bool Networking::isNumeric(const std::string& string) {
  static const std::regex numberRegex(
      R"(^[-+]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][-+]?\d+)?$)"
  );
  return std::regex_match(string, numberRegex);
}
