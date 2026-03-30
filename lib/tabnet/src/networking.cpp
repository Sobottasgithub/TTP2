#include "networking.h"

#include "methods.h"
#include "helpers.h"

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

helpers::Packet Networking::popOrder() {
  return popCollection(orderCollection);
}

helpers::Packet Networking::popSolution() {
  return popCollection(solutionCollection);
}

helpers::Packet Networking::popCollection(std::vector<helpers::Packet> collection) {
  std::lock_guard<std::mutex> lock(mtx);
  if (!collection.empty()) {
    helpers::Packet firstOrder = collection[0];
    collection.erase(collection.begin());  
    return firstOrder;
  }
  helpers::Packet emptyPacket;
  return emptyPacket;
}

void Networking::pushSolution(helpers::Packet solution) {
  std::lock_guard<std::mutex> lock(mtx);
  solutionCollection.push_back(solution);
}

void Networking::pushOrder(helpers::Packet order) {
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

