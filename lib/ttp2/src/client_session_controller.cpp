#include "../include/client_session_controller.h"

#include "../include/methods.h"

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <thread>

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

  std::thread sendRequestSessionThread([this]() {
      this->sendRequestSession();
  });

  std::thread receiveResponseSessionThread([this]() {
      this->sendRequestSession();
  });
 
  while (isConnected()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }

  if (sendRequestSessionThread.joinable()) {
    sendRequestSessionThread.join();
  }

  if (receiveResponseSessionThread.joinable()) {
    receiveResponseSessionThread.join();
  }
}

void ClientSessionController::receiveResponseSession() {
  while (isConnected()) {
    Packet packet = receiveMessage(socket);
    pushResponse(packet);
  }
}

void ClientSessionController::sendRequestSession() {
  while (isConnected()) {
    if (hasRequest()) {
      Packet request = popRequest();
      int responseCode = sendPacket(socket, request);
    }
  }
}
