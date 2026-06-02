#include "../include/networking.h"

#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <ifaddrs.h>
#include <iostream>
#include <mutex>
#include <net/if.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <variant>
#include <sstream>

extern "C" {
#include <libtasn1.h>
extern const asn1_static_node packets_asn1_tab[];
}

bool Networking::hasResponse() {
  std::lock_guard<std::mutex> lock(mtx);
  return !responseQueue.empty();
}

bool Networking::hasRequest() {
  std::lock_guard<std::mutex> lock(mtx);
  return !requestQueue.empty();
}

bool Networking::isConnected() {
  std::lock_guard<std::mutex> lock(mtx);
  return connected;
}

void Networking::disconnect() {
  std::lock_guard<std::mutex> lock(mtx);
  connected = false;
}

Networking::Packet Networking::popRequest() {
  std::lock_guard<std::mutex> lock(mtx);
  if (!requestQueue.empty()) {
    Networking::Packet firstRequest = requestQueue[0];
    requestQueue.erase(requestQueue.begin());
    return firstRequest;
  }
  Networking::Packet emptyPacket;
  return emptyPacket;
}

Networking::Packet Networking::popResponse() {
  std::lock_guard<std::mutex> lock(mtx);
  if (!responseQueue.empty()) {
    Networking::Packet firstResponse = responseQueue[0];
    responseQueue.erase(responseQueue.begin());
    return firstResponse;
  }
  Networking::Packet emptyPacket;
  return emptyPacket;
}

void Networking::pushResponse(Networking::Packet response) {
  std::lock_guard<std::mutex> lock(mtx);
  responseQueue.push_back(response);
}

void Networking::pushRequest(Networking::Packet request) {
  std::lock_guard<std::mutex> lock(mtx);
  requestQueue.push_back(request);
}

int Networking::getRequestQueueSize() {
  std::lock_guard<std::mutex> lock(mtx);
  return requestQueue.size();
}

int Networking::getResponseQueueSize() {
  std::lock_guard<std::mutex> lock(mtx);
  return responseQueue.size();
}

int Networking::sendPacket(int socket, Networking::Packet packet) {
  return sendMessage(socket, packet.id, packet.payload);
}

int Networking::sendMessage(int socket, int id,
                            payloadVariants payload) {
  asn1_node definitions = nullptr;
  asn1_node packet = nullptr;
  char errorDescription[ASN1_MAX_ERROR_DESCRIPTION_SIZE];

  if (asn1_array2tree(packets_asn1_tab, &definitions, errorDescription) !=
      ASN1_SUCCESS) {
    std::wcout << "Error in sendMessage when loading asn1:  "
               << errorDescription << std::endl;
    return -1;
  }

  asn1_create_element(definitions, "Packets.Packet", &packet);

  if (id == -1) {
    id = autoId;
    autoId++;
  }

  std::string idString = std::to_string(id);
  asn1_write_value(packet, "id", idString.c_str(), 0);

  if (std::holds_alternative<Standard>(payload)) {
    // Write structure
    int status = asn1_write_value(packet, "payload", "standard", 0);

    if (status != ASN1_SUCCESS) {
      std::wcout << "ASN1 set payload as standard failed!" << std::endl;
    }

    // Write contents
    std::string standardPayloadString = std::get<Standard>(payload).payload;
    const char *standardPayload = standardPayloadString.c_str();

    status = asn1_write_value(packet, "payload.standard.payload",
                              standardPayload, strlen(standardPayload));

    if (status != ASN1_SUCCESS) {
      std::wcout << "ASN1 set standard payload failed!" << std::endl;
    }
  } else if (std::holds_alternative<File>(payload)) {
    // Write structure
    int status = asn1_write_value(packet, "payload", "file", 0);

    if (status != ASN1_SUCCESS) {
      std::wcout << "ASN1 set payload as standard failed!" << std::endl;
    }

    // Write contents
    std::string filePathString = std::get<File>(payload).filePath;
    const char *filePath = filePathString.c_str();
    status = asn1_write_value(packet, "payload.file.filePath",
                              filePath, strlen(filePath));
    if (status != ASN1_SUCCESS) {
      std::wcout << "ASN1 set filepath failed!" << std::endl;
    }

    
    int start = std::get<File>(payload).start;
    status = asn1_write_value(packet, "payload.file.start",
                              &start, sizeof(start));
    if (status != ASN1_SUCCESS) {
      std::wcout << "ASN1 set file start failed!" << std::endl;
    }


    int end = std::get<File>(payload).end;
    status = asn1_write_value(packet, "payload.file.end",
                              &end, sizeof(end));
    if (status != ASN1_SUCCESS) {
      std::wcout << "ASN1 set file end failed!" << std::endl;
    }


    std::string filePayloadString = std::get<File>(payload).payload;
    const char *filePayload = filePayloadString.c_str();
    status = asn1_write_value(packet, "payload.file.payload",
                              filePayload, strlen(filePayload));
    if (status != ASN1_SUCCESS) {
      std::wcout << "ASN1 set file payload failed!" << std::endl;
    }
  }

  int derLen = 0;
  asn1_der_coding(packet, "", nullptr, &derLen, nullptr);
  std::vector<unsigned char> buffer(derLen);
  if (asn1_der_coding(packet, "", buffer.data(), &derLen, errorDescription) !=
      ASN1_SUCCESS) {
    std::wcout << "Error while encoding packet: " << errorDescription
               << std::endl;
    abort();
    return -1;
  }

  uint32_t size = htonl(derLen);
  sendBytes(socket, reinterpret_cast<char *>(&size), sizeof(size));
  sendBytes(socket, reinterpret_cast<char *>(buffer.data()), derLen);

  asn1_delete_structure(&packet);
  asn1_delete_structure(&definitions);

  return 0;
}

Networking::Packet Networking::receiveMessage(int socket) {
  Networking::Packet data;

  unsigned char temp[4096];
  while (true) {
    ssize_t n = recv(socket, temp, sizeof(temp), 0);
    if (n > 0) {
      sessionBuffers[socket].insert(sessionBuffers[socket].end(), temp,
                                    temp + n);
    } else if (n < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        break;
      }
      // std::wcout << "Error while receiving!" << std::endl;
      return data;
    } else {
      // std::wcout << "Socket closed!" << std::endl;
      return data;
    }
  }

  auto &buffer = sessionBuffers[socket];

  if (buffer.size() < sizeof(uint32_t)) {
    return data;
  }

  uint32_t networkSize;
  memcpy(&networkSize, buffer.data(), sizeof(uint32_t));
  uint32_t derLen = ntohl(networkSize);

  if (buffer.size() < sizeof(uint32_t) + derLen) {
    return data;
  }

  std::vector<char> derBuffer(buffer.begin() + sizeof(uint32_t),
                              buffer.begin() + sizeof(uint32_t) + derLen);

  buffer.erase(buffer.begin(), buffer.begin() + sizeof(uint32_t) + derLen);

  asn1_node definitions = nullptr;
  asn1_node packet = nullptr;
  char errorDescription[ASN1_MAX_ERROR_DESCRIPTION_SIZE];

  if (asn1_array2tree(packets_asn1_tab, &definitions, errorDescription) !=
      ASN1_SUCCESS) {
    return data;
  }

  asn1_create_element(definitions, "Packets.Packet", &packet);

  if (asn1_der_decoding(&packet, derBuffer.data(), derLen, errorDescription) ==
      ASN1_SUCCESS) {
    unsigned char idBin[8];
    int idLen = sizeof(idBin);
    if (asn1_read_value(packet, "id", idBin, &idLen) == ASN1_SUCCESS) {
      long idValue = 0;
      for (int i = 0; i < idLen; i++) {
        idValue = (idValue << 8) | idBin[i];
      }
      data.id = static_cast<int>(idValue);
    }

    char typeName[64];
    int branchSize = sizeof(typeName);
    int status = asn1_read_value(packet, "payload", typeName, &branchSize);
    std::string typeNameString = typeName;
    if (typeNameString == "standard") {
      int payloadLen = 0;
      asn1_read_value(packet, "payload.standard.payload", nullptr, &payloadLen);
      if (payloadLen > 0) {
        std::vector<char> payloadStr(payloadLen);
        asn1_read_value(packet, "payload.standard.payload", payloadStr.data(),
                        &payloadLen);

        Networking::Standard standard;
        standard.payload.assign(payloadStr.data(), payloadLen);

        data.payload = standard;
      }
    } else if (typeNameString == "file") {
        int filePathLen = 0;
        std::vector<char> filePathStr(filePathLen);
        asn1_read_value(packet, "payload.file.filePath", nullptr, &filePathLen);
        if (filePathLen > 0) {
          asn1_read_value(packet, "payload.file.filePath", filePathStr.data(),
                          &filePathLen);
        }

        int fileStartLen = 0;
        std::vector<char> fileStartBytes(fileStartLen);
        asn1_read_value(packet, "payload.file.start", nullptr, &fileStartLen);
        if (fileStartLen > 0) {
          asn1_read_value(packet, "payload.file.start", fileStartBytes.data(),
                          &fileStartLen);
        }
        int fileStart = bytesToInt(fileStartBytes, fileStartLen);

        int fileEndLen = 0;
        std::vector<char> fileEndBytes(fileEndLen);
        asn1_read_value(packet, "payload.file.end", nullptr, &fileEndLen);
        if (fileEndLen > 0) {
          asn1_read_value(packet, "payload.file.end", fileEndBytes.data(),
                          &fileEndLen);
        }
        int fileEnd = bytesToInt(fileEndBytes, fileEndLen);

        int payloadLen = 0;
        std::vector<char> payloadStr(payloadLen);
        asn1_read_value(packet, "payload.file.payload", nullptr, &payloadLen);
        if (payloadLen > 0) {
            asn1_read_value(packet, "payload.file.payload", payloadStr.data(),
                            &payloadLen);
        }
      
        Networking::File file;
        file.payload.assign(filePathStr.data(), filePathLen);
        file.start = fileStart;
        file.end = fileEnd;
        file.payload.assign(payloadStr.data(), payloadLen);

        data.payload = file;
    } else {
      std::wcout << "Error decoding payload: Unknown type!" << std::endl;
    }
  } else {
    std::wcout << "Error decoding ASN1" << std::endl;
  }

  asn1_delete_structure(&packet);
  asn1_delete_structure(&definitions);

  return data;
}

ssize_t Networking::sendBytes(int socket, const char *buffer, size_t max) {
  size_t sentBytes = 0;
  while (sentBytes < max) {
    ssize_t sent = send(socket, buffer + sentBytes, max - sentBytes, 0);

    if (sent <= 0)
      return -1;

    sentBytes += sent;
  }
  return sentBytes;
}

std::string Networking::getLocalIpAddress(std::string interface) {
  struct ifaddrs *ifaddr = nullptr;

  // Get linked list of network interfaces
  if (getifaddrs(&ifaddr) == -1) {
    return "";
  }

  std::string result;

  // Iterate through interfaces
  for (auto *ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
    if (!ifa->ifa_addr)
      continue;

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

std::string Networking::getBroadcastIpAddress() {
  struct ifaddrs *ifaddr = nullptr;
  std::string broadcastIP;

  // Get network interfaces
  if (getifaddrs(&ifaddr) == -1) {
    return "";
  }

  for (struct ifaddrs *ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
    if (ifa->ifa_addr == nullptr)
      continue;

    // Only consider IPv4 interfaces that are up and support broadcast
    if (ifa->ifa_addr->sa_family == AF_INET &&
        (ifa->ifa_flags & IFF_BROADCAST) && (ifa->ifa_flags & IFF_UP) &&
        !(ifa->ifa_flags & IFF_LOOPBACK)) {

      // Ensure the broadcast address exists
      if (ifa->ifa_broadaddr) {
        struct sockaddr_in *bcast =
            reinterpret_cast<struct sockaddr_in *>(ifa->ifa_broadaddr);
        char ip[INET_ADDRSTRLEN];
        if (inet_ntop(AF_INET, &(bcast->sin_addr), ip, INET_ADDRSTRLEN)) {
          broadcastIP = ip;
          break; // stop at the first valid one
        }
      }
    }
  }

  freeifaddrs(ifaddr);
  return broadcastIP;
}

bool Networking::isValidIpV4(std::string &ipString) {
  if (ipString.size() < 7)
    return false;

  int count = 0;
  // Seperate Ip Octets
  std::stringstream stringStream(ipString);
  while (stringStream.good()) {
    std::string octet;
    getline(stringStream, octet, '.');

    if (octet.size() > 1) {
      if (octet[0] == '0')
        return false;
    }

    for (int index = 0; index < octet.size(); index++) {
      if (isalpha(octet[index]))
        return false;
    }

    if (stoi(octet) > 255)
      return false;

    count++;
  }

  if (count != 4)
    return false;

  return true;
}

int Networking::bytesToInt(std::vector<char> bytes, int size) {
  int result = 0;
  for (int index = 0; index < size; index++)
  {
      result <<= 8;
      result |= (bytes[index] & 0xFF);
  }
  return result;
}
