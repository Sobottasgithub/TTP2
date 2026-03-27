#ifndef HELPERS_H
#define HELPERS_H

#include <string>
#include <netinet/in.h>
#include <sys/socket.h>

namespace helpers {
  struct Packet {
    int method;
    std::string payload;  
  };
  
  int sendMessage(int socket, int method, std::string payload);
  int sendPacket(int socket, Packet packet);
  Packet receiveMessage(int socket);
  bool isNumeric(const std::string& string);
}

#endif
