#include "../../include/core/udp_networking.h"

#include "../../include/helpers/asn1_encode.h"
#include "../../include/helpers/asn1_decode.h"

#include <bits/stdc++.h> 
#include <stdexcept>
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <netinet/in.h>

namespace ttp2 {
  void UdpNetworking::createServerSocket(const char *ip, int port) {
    struct sockaddr_in serverAddress; 
      
    if ((this->serverSocket = socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK, 0)) < 0) {
      throw std::runtime_error("Failed creating the server socket"); 
    }
      
    memset(&serverAddress, 0, sizeof(serverAddress)); 
      
    serverAddress.sin_family = AF_INET; 
    serverAddress.sin_addr.s_addr = inet_addr(ip); 
    serverAddress.sin_port = htons(port);
      
    if (bind(this->serverSocket,
        (const struct sockaddr *)&serverAddress,
        sizeof(serverAddress)) < 0 ) { 
      throw std::runtime_error("bind failed");
    }

    this->epollFd = epoll_create1(0);
    if (this->epollFd < 0) {
      close(this->serverSocket);
      throw std::runtime_error("epoll_create1 failed");
    }

    epoll_event event{};
    event.events = EPOLLIN | EPOLLET;
    event.data.fd = this->serverSocket;
    if (epoll_ctl(this->epollFd, EPOLL_CTL_ADD, this->serverSocket, &event) < 0) {
      close(this->serverSocket);
      close(this->epollFd);
      throw std::runtime_error("epoll_ctl failed");
    }
  }

  Packet::Packet UdpNetworking::receivePacketFrom() {
    Packet::Packet packet;
    sockaddr_in clientAddress{};
    socklen_t clientLength = sizeof(clientAddress);

    std::vector<char> buffer(1024);
    ssize_t bytes = recvfrom(this->serverSocket, buffer.data(), buffer.size(), MSG_PEEK | MSG_TRUNC, 
                             (struct sockaddr*)&clientAddress, &clientLength);

    if (bytes < 0) {
      std::cout << "Error while receiving" << std::endl;
      return packet;
    } else if (static_cast<size_t>(bytes) > buffer.size()) {
      buffer.resize(bytes);
      bytes = recvfrom(this->serverSocket, buffer.data(), buffer.size(), 0, 
                       (struct sockaddr*)&clientAddress, &clientLength);
    }

    // DEBUG
    char clientIP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(clientAddress.sin_addr), clientIP, INET_ADDRSTRLEN);
    uint16_t clientPort = ntohs(clientAddress.sin_port);
    std::cout << "Client IP: " << clientIP << std::endl;
    std::cout << "Client Port: " << clientPort << std::endl;
    std::cout << std::string(buffer.begin(), buffer.end()) << std::endl;

    packet = asn1::decode::decode(buffer);
    return packet;
  }

  void UdpNetworking::sendPacketTo(const char *ip, int port, ttp2::Packet::Packet packet) {
    std::vector<unsigned char> encodedPacket = ttp2::asn1::encode::encode(packet.payload, packet.id);
    const char* encodedData = reinterpret_cast<const char*>(encodedPacket.data());
    sendMessageTo(ip, port, encodedData);
  }

  void UdpNetworking::sendMessageTo(const char *ip, int port, const char *buffer) {
    int serverSocket;
    struct sockaddr_in serverAddress;

    serverSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (serverSocket < 0) {
      throw std::runtime_error("Socket creation failed");
    }

    memset(&serverAddress, 0, sizeof(serverAddress));

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = inet_addr(ip);

    sendto(serverSocket, buffer, strlen(buffer), MSG_CONFIRM,
           (const struct sockaddr *)&serverAddress, sizeof(serverAddress));

    std::cout << "SEND!" << std::endl;
  }
}
