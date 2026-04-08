#include "../include/client_session_controller.h"

#include "../include/methods.h"

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

extern "C" {
#include <libtasn1.h>
extern const asn1_static_node packets_asn1_tab[];
}


void ClientSessionController::testAsn1() {
  // Load asn1 schema from the _tab.c file
  asn1_node definitions = nullptr;
  int responseCode = asn1_array2tree(packets_asn1_tab, &definitions, nullptr);
  if (responseCode != ASN1_SUCCESS) {
      std::wcout << "Failed to load ASN.1 definitions" << std::endl;
      return;
  }
  std::wcout << "ASN.1 definitions loaded" << std::endl;

  // TEST: Packets.Packet
  asn1_node node = nullptr;
  responseCode = asn1_create_element(definitions, "Packets.Packet", &node);
  if (responseCode != ASN1_SUCCESS) {
      std::wcout << "Failed to create Packets.Packet" << std::endl;
      return;
  }

  int method = METHODS::test;
  std::string payload = "Hello world";
  asn1_write_value(node, "method", &method, sizeof(method));
  asn1_write_value(node, "payload", &payload, 1);

  std::wcout << "ASN.1 object filled with data!" << std::endl;

  // Cleanup
  asn1_delete_structure(&node);
  asn1_delete_structure(&definitions);
}

void ClientSessionController::networkingSession(int socket) {
  this->socket = socket;

  // Compleate Handshake
  Packet handshakePacket = receiveMessage(socket);
  if (handshakePacket.method != METHODS::handshake) {
    std::wcout << "Handshake failed!" << std::endl;
    connected = false;
    close(socket);
    return;
  }

  int responseCode = 0;
  while (responseCode >= 0) {
    // Receive solution(s)
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
      
    // Send order(s)
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
