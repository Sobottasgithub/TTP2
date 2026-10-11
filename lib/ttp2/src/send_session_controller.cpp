#include "../include/send_session_controller.h"

#include "../include/packet_types.h"

#include <tablog.h>

namespace ttp2 {
  SendSessionController::SendSessionController(int clientSocket) {
    configureLogger("TTP2");
    this->clientSocket = clientSocket;
  }

  void SendSessionController::sendResponseSession() {
    while (isConnected()) {
      if (hasResponse()) {
        Packet::Packet responsePacket = popResponse();
        int responseCode = sendPacket(clientSocket, responsePacket);
      }
    }
  }

  void SendSessionController::disconnect() {
    std::lock_guard<std::mutex> lock(mtx);
    close(this->clientSocket);
    connected = false;
  }
}
