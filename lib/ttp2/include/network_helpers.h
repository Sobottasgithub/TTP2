#ifndef NETWORK_HELPERS_H
#define NETWORK_HELPERS_H

#include <string>

namespace ttp2::helpers {
  std::string getBroadcastIpAddress();
  std::string getLocalIpAddress(std::string interface);
  bool isValidIpV4(std::string &ipString);
  bool isValidInterface(std::string &interface);
}

#endif
