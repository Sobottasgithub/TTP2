#include "../include/networking.h"
#include "../include/asn1_helpers.h"
#include "../include/asn1_encode.h"

#include <tablog_registry.h>
#include <tablog.h>

#include <arpa/inet.h>
#include <cstdlib>
#include <cstring>
#include <ifaddrs.h>
#include <memory>
#include <mutex>
#include <net/if.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <variant>
#include <sstream>
#include <ifaddrs.h>
#include <arrow/api.h>
#include <arrow/ipc/api.h>
#include <arrow/io/api.h>
#include <arrow/csv/api.h>
#include <arrow/buffer.h>
#include <arrow/io/memory.h>
#include <arrow/ipc/reader.h>
#include <arrow/ipc/writer.h>
#include <arrow/result.h>
#include <arrow/status.h>
#include <arrow/table.h>


extern "C" {
#include <libtasn1.h>
extern const asn1_static_node packets_asn1_tab[];
}

namespace ttp2 {
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
    if (id == -1) {
      id = autoId;
      autoId++;
    }

    std::vector<unsigned char> buffer = tql::asn1::encode::encode(payload, id);
    
    uint32_t size = htonl(buffer.size());
    sendBytes(socket, reinterpret_cast<char *>(&size), sizeof(size));
    sendBytes(socket, reinterpret_cast<char *>(buffer.data()), buffer.size());

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
        logger->log(tablog::ERROR, "Error while receiving bytes!");
        return data;
      } else {
        // logger->log(tablog::CRITICAL, "Socket closed!");
        disconnect();
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
      data.id = Asn1Helpers::asn1DecodePayloadInt(packet, "id");

      char typeName[64];
      int branchSize = sizeof(typeName);
      int status = asn1_read_value(packet, "payload", typeName, &branchSize);
      std::string typeNameString = typeName;
      if (typeNameString == "standard") {
        Networking::Standard standard;
        standard.payload = Asn1Helpers::asn1DecodePayloadString(packet, "payload.standard.payload");

        data.payload = standard;
      } else if (typeNameString == "file") {
        Networking::File file;
        file.filePath = Asn1Helpers::asn1DecodePayloadString(packet, "payload.file.filePath");
        file.start = Asn1Helpers::asn1DecodePayloadInt(packet, "payload.file.start");
        file.end = Asn1Helpers::asn1DecodePayloadInt(packet, "payload.file.end");

        std::vector<uint8_t> buffer = Asn1Helpers::asn1DecodePayloadBuffer(packet, "payload.file.payload");
        // const uint8_t* bufferConst = buffer.data();
        file.payload = bufferToTable(buffer.data(), buffer.size());

        data.payload = file;
      } else if (typeNameString == "viewportRequest") {
        Networking::ViewportRequest viewportRequest;

        viewportRequest.xStart = Asn1Helpers::asn1DecodePayloadInt(packet, "payload.viewportRequest.xStart");
        viewportRequest.xEnd = Asn1Helpers::asn1DecodePayloadInt(packet, "payload.viewportRequest.xEnd");
        viewportRequest.yStart = Asn1Helpers::asn1DecodePayloadInt(packet, "payload.viewportRequest.yStart");
        viewportRequest.yEnd = Asn1Helpers::asn1DecodePayloadInt(packet, "payload.viewportRequest.yEnd");

        data.payload = viewportRequest;
      } else if (typeNameString == "viewport") {
        Networking::Viewport viewport;
        viewport.xStart = Asn1Helpers::asn1DecodePayloadInt(packet, "payload.viewport.xStart");
        viewport.xEnd = Asn1Helpers::asn1DecodePayloadInt(packet, "payload.viewport.xEnd");
        viewport.yStart = Asn1Helpers::asn1DecodePayloadInt(packet, "payload.viewport.yStart");
        viewport.yEnd = Asn1Helpers::asn1DecodePayloadInt(packet, "payload.viewport.yEnd");
        std::vector<uint8_t> buffer = Asn1Helpers::asn1DecodePayloadBuffer(packet, "payload.viewport.payload");
        viewport.payload = bufferToTable(buffer.data(), buffer.size());

        data.payload = viewport;
      } else if (typeNameString == "tqlQuery") {
        Networking::TqlQuery tqlQuery;
        tqlQuery.query = Asn1Helpers::asn1DecodePayloadString(packet, "payload.tqlQuery.query");

        data.payload = tqlQuery;
      } else {
        logger->log(tablog::ERROR, "Error decoding payload: Unknown type!");
      }
    } else {
      logger->log(tablog::ERROR, "Error decoding ASN1");
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

  Networking::PacketInfo Networking::peekResponse() {
    return peekResponse(0);
  }

  Networking::PacketInfo Networking::peekResponse(int index) {
    Networking::PacketInfo packetInfo;
    if (index > getResponseQueueSize() - 1 || index < 0) {
      logger->log(tablog::ERROR, "Invalid peek request: " + std::to_string(index));
      return packetInfo;
    }

    std::lock_guard<std::mutex> lock(mtx);
    packetInfo.id = responseQueue[index].id;
    packetInfo.payloadType = responseQueue[index].payload;
    return packetInfo;
  }

  Networking::PacketInfo Networking::peekRequest() {
    return peekRequest(0);
  }

  Networking::PacketInfo Networking::peekRequest(int index) {
    Networking::PacketInfo packetInfo;
    if (index > getRequestQueueSize() - 1 || index < 0) {
      logger->log(tablog::ERROR, "Invalid peek request: " + std::to_string(index));
      return packetInfo;
    }

    std::lock_guard<std::mutex> lock(mtx);
    packetInfo.id = requestQueue[index].id;
    packetInfo.payloadType = requestQueue[index].payload;
    return packetInfo;
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

  bool Networking::isValidInterface(std::string &interface) {
      struct ifaddrs *addresses;
      getifaddrs(&addresses);

      bool isValid = false;
      for (struct ifaddrs *address = addresses; address != nullptr; address = address->ifa_next) {
          if (address->ifa_addr && address->ifa_addr->sa_family == AF_PACKET) {
              if (address->ifa_name == interface) {
                  isValid = true;
              }
          }
      }

      freeifaddrs(addresses);
      return isValid;
  }

  std::shared_ptr<arrow::Table> Networking::bufferToTable(const uint8_t* rawData, int64_t dataSize) {
    arrow::BufferBuilder bufferBuilder;
    arrow::Status allocStatus = bufferBuilder.Resize(dataSize);
    if (!allocStatus.ok()) {
      logger->log(tablog::ERROR, "Buffer allocation failed in bufferToTable");
      return arrow::Table::Make(arrow::schema({}), std::vector<std::shared_ptr<arrow::Array>>{});
    }

    // Make a physical copy so that the data isn't deleted. (That would lead to a shared_ptr with a table that points to no real data)
    arrow::Status appendStatus = bufferBuilder.Append(reinterpret_cast<const uint8_t*>(rawData), dataSize);
    if (!appendStatus.ok()) {
      logger->log(tablog::ERROR, "Failed to append raw data to buffer");
      return arrow::Table::Make(arrow::schema({}), std::vector<std::shared_ptr<arrow::Array>>{});
    }

    std::shared_ptr<arrow::Buffer> buffer;
    arrow::Status finishStatus = bufferBuilder.Finish(&buffer);
    if (!finishStatus.ok()) {
      logger->log(tablog::ERROR, "Failed to finish buffer building");
      return arrow::Table::Make(arrow::schema({}), std::vector<std::shared_ptr<arrow::Array>>{});
    }

    std::shared_ptr<arrow::io::InputStream> inputStream = std::make_shared<arrow::io::BufferReader>(buffer);

    arrow::Result<std::shared_ptr<arrow::ipc::RecordBatchStreamReader>> streamReaderResult = arrow::ipc::RecordBatchStreamReader::Open(inputStream);
    if (!streamReaderResult.ok()) {
      logger->log(tablog::ERROR, "Open input stream failed in bufferToTable");
      return arrow::Table::Make(arrow::schema({}), std::vector<std::shared_ptr<arrow::Array>>{});
    }
    std::shared_ptr<arrow::ipc::RecordBatchStreamReader> streamReader = std::move(streamReaderResult).ValueUnsafe();

    arrow::Result<std::shared_ptr<arrow::Table>> tableResult = streamReader->ToTable();
    if (!tableResult.ok()) {
      logger->log(tablog::ERROR, "Create table failed in bufferToTable");
      return arrow::Table::Make(arrow::schema({}), std::vector<std::shared_ptr<arrow::Array>>{});
    }

    return *tableResult;
  }

  void Networking::disconnect() {}

  void Networking::configureLogger(std::string name) {
    tablog::TablogRegistry* registry = &tablog::TablogRegistry::getInstance();
    std::shared_ptr<tablog::Tablog> logger = std::make_shared<tablog::Tablog>();
    logger->configure(name, true);
    registry->registerLogger(name, logger);
    this->logger = logger;
  }
}
