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

  std::tuple<std::string, int, Packet::Packet> UdpNetworking::receivePacketFrom() {
    Packet::Packet packet;
    sockaddr_in clientAddress{};
    socklen_t clientLength = sizeof(clientAddress);

    ssize_t bytes = recvfrom(this->serverSocket, nullptr, 0, MSG_PEEK | MSG_TRUNC, 
                             (struct sockaddr*)&clientAddress, &clientLength);
    if (bytes < 0) {
      std::cout << "Error while receiving" << std::endl;
      return {"", -1, packet};
    }
    std::vector<unsigned char> buffer(bytes);
    bytes = recvfrom(this->serverSocket, buffer.data(), buffer.size(), 0, 
                     (struct sockaddr*)&clientAddress, &clientLength);
    if (bytes < 0) {
      std::cout << "Error while consuming packet" << std::endl;
      return {"", -1, packet};
    }
    
    char clientIP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(clientAddress.sin_addr), clientIP, INET_ADDRSTRLEN);
    uint16_t clientPort = ntohs(clientAddress.sin_port);

    uint32_t networkSize;
    memcpy(&networkSize, buffer.data(), sizeof(uint32_t));
    uint32_t derLen = ntohl(networkSize);
    std::vector<char> derBuffer(
        reinterpret_cast<const char*>(buffer.data() + sizeof(uint32_t)),
        reinterpret_cast<const char*>(buffer.data() + sizeof(uint32_t) + derLen)
    );

    packet = asn1::decode::decode(derBuffer);
    return {clientIP, clientPort, packet};
  }

  void UdpNetworking::sendPacketTo(const char *ip, int port, ttp2::Packet::Packet packet) {
    std::vector<unsigned char> encodedPacket = ttp2::asn1::encode::encode(packet.payload, packet.id);

    uint32_t dataLen = static_cast<uint32_t>(encodedPacket.size());
    uint32_t networkSize = htonl(dataLen);

    std::vector<unsigned char> finalBuffer;
    finalBuffer.resize(sizeof(uint32_t) + encodedPacket.size());

    std::memcpy(finalBuffer.data(), &networkSize, sizeof(uint32_t));
    std::memcpy(finalBuffer.data() + sizeof(uint32_t), encodedPacket.data(), encodedPacket.size());

    sendMessageTo(ip,
                  port,
                  reinterpret_cast<const char*>(finalBuffer.data()), 
                  finalBuffer.size()
                 );
  }
  
  void UdpNetworking::sendMessageTo(const char *ip, int port, const char *buffer, size_t size) {
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

    sendto(serverSocket, buffer, size, MSG_CONFIRM,
           (const struct sockaddr *)&serverAddress, sizeof(serverAddress));
  }
}
