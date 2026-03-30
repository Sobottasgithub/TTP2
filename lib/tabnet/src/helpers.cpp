#include "helpers.h"

#include "methods.h"

#include <string>
#include <regex>

namespace helpers {
  int sendPacket(int socket, Packet packet) {
      return sendMessage(socket, packet.method, packet.payload);
  }
  
  int sendMessage(int socket, int method, std::string payload) {
    return 0;
  }

  Packet receiveMessage(int socket) {
      Packet data;      
      return data;
  }

  bool isNumeric(const std::string& string) {
    static const std::regex numberRegex(
        R"(^[-+]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][-+]?\d+)?$)"
    );
    return std::regex_match(string, numberRegex);
  }
}
