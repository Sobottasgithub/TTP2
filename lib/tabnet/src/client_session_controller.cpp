#include "../include/client_session_controller.h"

#include "../include/methods.h"

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

void ClientSessionController::networkingSession(int serverSocket, int clientSocket) {
  
  // Compleate Handshake
  int responseCode = ClientSessionController::sendMessage(clientSocket, METHODS::handshake, "");
  if (responseCode < 0) {
    std::wcout << "Socket: " << clientSocket << " closed during the handshake!" << std::endl;
    close(clientSocket);
    connected = false;
    return;
  }

  while (true) {
    // Hand back finished solution 
    int solutionCollectionSize = getSolutionCollectionSize();
    responseCode = sendMessage(clientSocket, METHODS::size, std::to_string(solutionCollectionSize));
    ClientSessionController::Packet response = receiveMessage(clientSocket);
    if (response.method == METHODS::success) {
      for(int index = 0; index < solutionCollectionSize; index++) {
        // Send data
        ClientSessionController::Packet solution = popSolution();
        responseCode = sendPacket(clientSocket, solution);
        ClientSessionController::Packet response = receiveMessage(clientSocket);
        if (response.method != METHODS::success) {
          std::wcout << "Expected: " << METHODS::success << " (success) or " << METHODS::failed << " (failed), but got " << response.method << std::endl;
          std::wcout << "With following payload" << response.payload.c_str() << std::endl;
        }
      }
    } else if (response.method == METHODS::failed) {
      std::wcout << "Something went wrong while sending the size! Master response: " << response.payload.c_str() << std::endl;
    } else {
      std::wcout << "Expected: " << METHODS::success << " (success) or " << METHODS::failed << " (failed), but got " << response.method << std::endl;
      std::wcout << "With following payload" << response.payload.c_str() << std::endl;
    }

    responseCode = sendMessage(clientSocket, METHODS::ready, "");
    
    //receive
    ClientSessionController::Packet receivedPacket = receiveMessage(clientSocket);
    if (receivedPacket.method == METHODS::size) {
      int count = std::stoi(receivedPacket.payload);
      if (count != 0) {
        responseCode = sendMessage(clientSocket, METHODS::success, "");
        for(int i = 0; i < count; i++) {
          Packet order = receiveMessage(clientSocket);
          pushOrder(order);
          responseCode = sendMessage(clientSocket, METHODS::success, "");
        }
      }
    } else {
      std::wcout << "Expected: " << METHODS::size << " (size) got: " << receivedPacket.method << std::endl;
      responseCode = sendMessage(clientSocket, METHODS::failed, "Expected method size");
    }

    // Controlled shutdown of this thread, if the master crashes
    if (responseCode < 0) {
      std::wcout << "Socket: " << clientSocket << " closed!" << std::endl;
      close(clientSocket);
      return;
    }    
  }
}
