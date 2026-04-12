#include "../include/client_session_controller.h"

#include "../include/methods.h"

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

ClientSessionController::ClientSessionController(int &socket) {
  std::wcout << "Constructor of networking called!" << std::endl;
  this->socket = socket;
}


void ClientSessionController::networkingSession() {
  std::wcout << "NETWORKING SESSION" << std::endl;
  // Compleate Handshake
  Packet handshakePacket = receiveMessage(socket);
  int responseCode = sendMessage(socket, METHODS::handshake, "hello");
  if (handshakePacket.method != METHODS::handshake) {
    std::wcout << "Handshake failed!" << std::endl;
    connected = false;
    close(socket);
    return;
  }

  std::wcout << "after handshake" << std::endl;
  
  responseCode = 0;
  while (responseCode >= 0) {
    std::wcout << "Cycle1" << std::endl;
    // Receive solution(s)
    Packet solutionCount = receiveMessage(socket);
    std::wcout << "Helloi world!" << std::endl;
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
      
    // Send order(s)
    int orderCollectionSize = getOrderCollectionSize();
    responseCode = sendMessage(socket, METHODS::size, std::to_string(orderCollectionSize));
    std::wcout << "cycle send" << std::endl;
    if (orderCollectionSize > 0) {
      std::wcout << "orderColelctionSize: " << orderCollectionSize << std::endl;
      if (receiveMessage(socket).method == METHODS::success) {
        for(int index = 0; index < orderCollectionSize; index++) {
          responseCode = sendPacket(socket, popOrder());
          Packet response = receiveMessage(socket);
          std::wcout << "send order!" << std::endl;
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
        std::wcout << "Client shut down!" << std::endl;
        break;
    }
  }
  connected = false;
}
