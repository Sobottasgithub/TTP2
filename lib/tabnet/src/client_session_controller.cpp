#include "../include/client_session_controller.h"

#include "../include/methods.h"

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

ClientSessionController::ClientSessionController(int &socket) {
  this->socket = socket;
}

void ClientSessionController::networkingSession() {
  // Compleate Handshake
  Packet handshakePacket = receiveMessage(socket);
  int responseCode = sendMessage(socket, METHODS::handshake, "");
  if (handshakePacket.method != METHODS::handshake) {
    std::wcout << "Handshake failed!" << std::endl;
    connected = false;
    close(socket);
    return;
  }
 
  responseCode = 0;
  while (responseCode >= 0) {
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
    int requestCollectionSize = getRequestCollectionSize();
    responseCode = sendMessage(socket, METHODS::size, std::to_string(requestCollectionSize));
    if (requestCollectionSize > 0) {
      if (receiveMessage(socket).method == METHODS::success) {
        for(int index = 0; index < requestCollectionSize; index++) {
          responseCode = sendPacket(socket, popRequest());
          Packet response = receiveMessage(socket);
          if (response.method != METHODS::success) {
            std::wcout << "Send request to node failed: got " << response.method << std::endl;
          }
        }
        responseCode = sendMessage(socket, METHODS::ready, "");
      } else {
        std::wcout << "Send of size failed!" << std::endl;
        responseCode = sendMessage(socket, METHODS::ready, "");
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
