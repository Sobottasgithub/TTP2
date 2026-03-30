#include "../include/tabcrypt.h"

#include "../include/methods.h"

#include <iostream>


void ServerSessionController::networkingSession(int socket) {
  this->socket = socket;

  int responseCode = 0;
  while (responseCode >= 0) {
    // Receive solutions
    helpers::Packet solutionCount = helpers::receiveMessage(socket);
    if (solutionCount.method == METHODS::size) {
      responseCode = helpers::sendMessage(socket, METHODS::success, "");
      for (int index = 0; index < std::stoi(solutionCount.payload); index++) {
        helpers::Packet packet = helpers::receiveMessage(socket);
        pushSolution(packet);
        responseCode = helpers::sendMessage(socket, METHODS::success, "");
      }
    } else {
      responseCode = helpers::sendMessage(socket, METHODS::failed, "");
      std::wcout << "Something went wrong during receiving size!" << std::endl;
      std::wcout << "Got: " << solutionCount.method << " instead of " << METHODS::size << " (size)" << std::endl;
    }

    helpers::Packet ready = helpers::receiveMessage(socket);
      
    // Send orders
    int orderCollectionSize = getOrderCollectionSize();
    responseCode = helpers::sendMessage(socket, METHODS::size, std::to_string(orderCollectionSize));
    if (orderCollectionSize > 0) {
      if (helpers::receiveMessage(socket).method == METHODS::success) {
        for(int index = 0; index < orderCollectionSize; index++) {
          responseCode = helpers::sendPacket(socket, popOrder());
          helpers::Packet response = helpers::receiveMessage(socket);
          if (response.method != METHODS::success) {
            std::wcout << "Send order to node failed: got " << response.method << std::endl;
          }
        }
      } else {
        std::wcout << "Send of size failed!" << std::endl;
      }
    }

    if (responseCode < 0) {
        responseCode = 0;
        connected = false;
        break;
    }
  }
  connected = false;
}
