#include "../include/server_session_controller.h"

#include "../include/methods.h"

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <thread>

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

  std::thread receiveRequestSessionThread([this]() {
      this->receiveRequestSession();
  });

  std::thread sendResponseSessionThread([this]() {
      this->sendResponseSession();
  });
 
  while (isConnected()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
  }

  if (receiveRequestSessionThread.joinable()) {
    receiveRequestSessionThread.join();
  }

  if (sendResponseSessionThread.joinable()) {
    sendResponseSessionThread.join();
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

void ServerSessionController::sendResponseSession() {
  while (isConnected()) {
    if (hasResponse()) {
      Packet responsePacket = popResponse();
      int responseCode = sendPacket(clientSocket, responsePacket);
      Packet response = receiveMessage(clientSocket);
      if (response.method != METHODS::success) {
        std::wcout << "Expected: " << METHODS::success << " (success) or " << METHODS::failed << " (failed), but got " << response.method << std::endl;
        std::wcout << "With following payload" << response.payload.c_str() << std::endl;
      }
      validateConnection(responseCode);
    }
  }
}

void ServerSessionController::receiveRequestSession() {
  while (isConnected()) {
    Packet packet = receiveMessage(clientSocket);
    pushRequest(packet);
    int responseCode = sendMessage(clientSocket, METHODS::success, "");
    validateConnection(responseCode);
  }
}

void ServerSessionController::validateConnection(int responseCode) {
  if (responseCode < 0) {
      std::wcout << "Stream failed with response code: " << responseCode << std::endl;
      disconnect();
      close(clientSocket);
  }
}

