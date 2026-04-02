#include "../include/server_session_controller.h"

#include "../include/methods.h"

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>


void ServerSessionController::networkingSession(int socket) {
  this->socket = socket;

  // Compleate Handshake
  ServerSessionController::Packet handshakePacket = receiveMessage(socket);
  if (handshakePacket.method != METHODS::handshake) {
    std::wcout << "Handshake failed!" << std::endl;
    connected = false;
    close(socket);
    return;
  }

  int responseCode = 0;
  while (responseCode >= 0) {
    // Receive solutions
    Packet solutionCount = receiveMessage(socket);
    if (solutionCount.method == METHODS::size) {
      responseCode = sendMessage(socket, METHODS::success, "");
      for (int index = 0; index < std::stoi(solutionCount.payload); index++) {
        Packet packet = receiveMessage(socket);
        pushSolution(packet);
        responseCode = sendMessage(socket, METHODS::success, "");
      }
    } else {
      responseCode = sendMessage(socket, METHODS::failed, "");
      std::wcout << "Something went wrong during receiving size!" << std::endl;
      std::wcout << "Got: " << solutionCount.method << " instead of " << METHODS::size << " (size)" << std::endl;
    }

    Packet ready = receiveMessage(socket);
      
    // Send orders
    int orderCollectionSize = getOrderCollectionSize();
    responseCode = sendMessage(socket, METHODS::size, std::to_string(orderCollectionSize));
    if (orderCollectionSize > 0) {
      if (receiveMessage(socket).method == METHODS::success) {
        for(int index = 0; index < orderCollectionSize; index++) {
          responseCode = sendPacket(socket, popOrder());
          Packet response = receiveMessage(socket);
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
