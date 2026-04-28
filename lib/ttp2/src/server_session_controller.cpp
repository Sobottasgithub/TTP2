#include "../include/server_session_controller.h"

#include "../include/methods.h"

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <arpa/inet.h>

ServerSessionController::ServerSessionController() {}

ServerSessionController::ServerSessionController(int serverSocket, int clientSocket) {
  this->serverSocket = serverSocket;
  this->clientSocket = clientSocket;
}

void ServerSessionController::networkingSession() {
  // Compleate Handshake
  int responseCode = sendMessage(clientSocket, METHODS::handshake, "");
  Packet handshakePacket = receiveMessage(clientSocket);
  if (responseCode < 0) {
    std::wcout << "Socket: " << clientSocket << " closed during the handshake!" << std::endl;
    close(clientSocket);
    disconnect();
    return;
  }

  while (isConnected()) {
    // Send response(s)
    int responseQueueSize = getResponseQueueSize();
    responseCode = sendMessage(clientSocket, METHODS::size, std::to_string(responseQueueSize));
    Packet response = receiveMessage(clientSocket);
    if (response.method == METHODS::success) {
      for(int index = 0; index < responseQueueSize; index++) {
        // Send data
        Packet responsePacket = popResponse();
        responseCode = sendPacket(clientSocket, responsePacket);
        response = receiveMessage(clientSocket);
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
    
    // Receive request(s)
    Packet requestCount = receiveMessage(clientSocket);
    if (requestCount.method == METHODS::size) {
      responseCode = sendMessage(clientSocket, METHODS::success, "");
      for (int index = 0; index < std::stoi(requestCount.payload); index++) {
        Packet packet = receiveMessage(clientSocket);
        pushRequest(packet);
        responseCode = sendMessage(clientSocket, METHODS::success, "");
      }
    } else {
      responseCode = sendMessage(clientSocket, METHODS::failed, "");
      std::wcout << "Something went wrong during receiving size!" << std::endl;
      std::wcout << "Got: " << requestCount.method << " instead of " << METHODS::size << " (size)" << std::endl;
    }

    response = receiveMessage(clientSocket); // receive ready
    
    // Controlled shutdown of this thread, if the master crashes
    if (responseCode < 0) {
      std::wcout << "Socket: " << clientSocket << " closed!" << std::endl;
      disconnect();
      close(clientSocket);
      return;
    }
  }
}

std::string ServerSessionController::getLocalIpAddress(std::string interface) {
  struct ifaddrs *ifaddr = nullptr;

  // Get linked list of network interfaces
  if (getifaddrs(&ifaddr) == -1) {
      return "";
  }

  std::string result;

  // Iterate through interfaces
  for (auto *ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
      if (!ifa->ifa_addr) continue;

      if (ifa->ifa_addr->sa_family == AF_INET) {
          auto *addr = reinterpret_cast<struct sockaddr_in *>(ifa->ifa_addr);
          char ip[INET_ADDRSTRLEN];
          inet_ntop(AF_INET, &addr->sin_addr, ip, sizeof(ip));

          // Docker containers typically use eth0
          if (std::string(ifa->ifa_name) == interface) {
              result = ip;
              break;
          }
      }
  }

  freeifaddrs(ifaddr);
  return result;
}
