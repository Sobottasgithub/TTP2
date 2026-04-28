#include "../include/client_session_controller.h"

#include "../include/methods.h"

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

ClientSessionController::ClientSessionController() {}

ClientSessionController::ClientSessionController(int &socket) {
  this->socket = socket;
}

void ClientSessionController::networkingSession() {
  // Compleate Handshake
  Packet handshakePacket = receiveMessage(socket);
  int responseCode = sendMessage(socket, METHODS::handshake, "");
  if (handshakePacket.method != METHODS::handshake) {
    std::wcout << "Handshake failed!" << std::endl;
    disconnect();
    close(socket);
    return;
  }
 
  responseCode = 0;
  while (isConnected()) {
    // Receive response(s)
    Packet responseCount = receiveMessage(socket);
    if (responseCount.method == METHODS::size) {
      responseCode = sendMessage(socket, METHODS::success, "");
      for (int index = 0; index < std::stoi(responseCount.payload); index++) {
        Packet packet = receiveMessage(socket);
        pushResponse(packet);
        responseCode = sendMessage(socket, METHODS::success, "");
      }
    } else {
      responseCode = sendMessage(socket, METHODS::failed, "");
      std::wcout << "Something went wrong during receiving size!" << std::endl;
      std::wcout << "Got: " << responseCount.method << " instead of " << METHODS::size << " (size)" << std::endl;
    }

    Packet ready = receiveMessage(socket);
      
    // Send request(s)
    int requestQueueSize = getRequestQueueSize();
    responseCode = sendMessage(socket, METHODS::size, std::to_string(requestQueueSize));
    Packet response = receiveMessage(socket);
    if (response.method == METHODS::success) {
      for(int index = 0; index < requestQueueSize; index++) {
        // Send data
        Packet request = popRequest();
        responseCode = sendPacket(socket, request);
        response = receiveMessage(socket);
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
    
    responseCode = sendMessage(socket, METHODS::ready, "");

    if (responseCode < 0) {
        std::wcout << "Stream failed with response code: " << responseCode << std::endl;
        disconnect();
        close(socket);
        break;
    }
  }
  connected = false;
}
