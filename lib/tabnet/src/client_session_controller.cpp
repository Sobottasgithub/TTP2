#include "../include/client_session_controller.h"

#include "../include/methods.h"

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

extern "C" {
#include "asn1/Packet.h"
#include "asn1/asn_application.h"
#include "asn1/asn_internal.h"
}

static int ClientSessionController::write_callback(const void *buffer, size_t size, void *app_key) {
    auto *vec = reinterpret_cast<std::vector<uint8_t> *>(app_key);
    const uint8_t *data = reinterpret_cast<const uint8_t *>(buffer);

    vec->insert(vec->end(), data, data + size);
    return 0;
}

void ClientSessionController::testAsn1() {
  std::wcout << "Start test ASN1" << std::endl;
  Packet_t *msg = (Packet_t*)calloc(1, sizeof(Packet_t));

  msg->method = 41;
  const char *payload = "Hello world!";
  OCTET_STRING_fromBuf(&msg.payload, payload, strlen(payload));

 std::vector<uint8_t> bytes;

  asn_enc_rval_t rval = der_encode(
      &asn_DEF_TestMessage,
      &msg,
      write_callback,
      &bytes
  );

  if (rval.encoded == -1) {
      std::cerr << "Encode failed\n";
      return 1;
  }

  std::wcout << "Encoded bytes: " << bytes.size() << "\n";

  for (auto b : bytes)
      printf("%02X ", b);

  printf("\n");
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
