#include "../include/networking.h"
#include "../include/asn1_helpers.h"

#include <arpa/inet.h>
#include <arrow/buffer.h>
#include <arrow/io/memory.h>
#include <arrow/ipc/reader.h>
#include <arrow/ipc/writer.h>
#include <arrow/result.h>
#include <arrow/status.h>
#include <arrow/table.h>
#include <cstdlib>
#include <cstring>
#include <ifaddrs.h>
#include <iostream>
#include <memory>
#include <mutex>
#include <net/if.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <system_error>
#include <variant>
#include <sstream>
#include <ifaddrs.h>
#include <arrow/api.h>
#include <arrow/ipc/api.h>
#include <arrow/io/api.h>
#include <arrow/csv/api.h>

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

    packet = Asn1Helpers::asn1EncodePayload(id, packet, "id");

    if (std::holds_alternative<Standard>(payload)) {
      // Write structure
      int status = asn1_write_value(packet, "payload", "standard", 0);

      if (status != ASN1_SUCCESS) {
        std::wcout << "ASN1 set payload as standard failed!" << std::endl;
      }

      // Write contents
      std::string standardPayloadString = std::get<Standard>(payload).payload;
      packet = Asn1Helpers::asn1EncodePayload(standardPayloadString, packet, "payload.standard.payload");

    } else if (std::holds_alternative<File>(payload)) {
      // Write structure
      int status = asn1_write_value(packet, "payload", "file", 0);

      if (status != ASN1_SUCCESS) {
        std::wcout << "ASN1 set payload as file failed!" << std::endl;
      }

      // Write contents
      std::string filePathString = std::get<File>(payload).filePath;
      packet = Asn1Helpers::asn1EncodePayload(filePathString, packet, "payload.file.filePath");

      int start = std::get<File>(payload).start;
      packet = Asn1Helpers::asn1EncodePayload(start, packet, "payload.file.start");

      int end = std::get<File>(payload).end;
      packet = Asn1Helpers::asn1EncodePayload(end, packet, "payload.file.end");

      std::shared_ptr<arrow::Table> table = std::get<File>(payload).payload;
      std::shared_ptr<arrow::Buffer> buffer = tableToBuffer(table);
      packet = Asn1Helpers::asn1EncodePayload(buffer->data(), buffer->size(), packet, "payload.file.payload");
    } else if (std::holds_alternative<Viewport>(payload)) {
      // Write structure
      int status = asn1_write_value(packet, "payload", "viewport", 0);

      if (status != ASN1_SUCCESS) {
        std::wcout << "ASN1 set payload as viewport failed!" << std::endl;
      }

      // Write content
      // X
      int xStart = std::get<Viewport>(payload).xStart;
      packet = Asn1Helpers::asn1EncodePayload(xStart, packet, "payload.viewport.xStart");

      int xEnd = std::get<Viewport>(payload).xEnd;
      packet = Asn1Helpers::asn1EncodePayload(xEnd, packet, "payload.viewport.xEnd");

      // Y
      int yStart = std::get<Viewport>(payload).yStart;
      packet = Asn1Helpers::asn1EncodePayload(yStart, packet, "payload.viewport.yStart");

      int yEnd = std::get<Viewport>(payload).yEnd;
      packet = Asn1Helpers::asn1EncodePayload(yEnd, packet, "payload.viewport.yEnd");

      std::shared_ptr<arrow::Table> table = std::get<Viewport>(payload).payload;
      std::shared_ptr<arrow::Buffer> buffer = tableToBuffer(table);
      packet = Asn1Helpers::asn1EncodePayload(buffer->data(), buffer->size(), packet, "payload.viewport.payload");
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
        std::wcout << "Error while receiving bytes!" << std::endl;
        return data;
      } else {
        // std::wcout << "Socket closed!" << std::endl;
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
      } else if (typeNameString == "viewport") {
          Networking::Viewport viewport;
          viewport.xStart = Asn1Helpers::asn1DecodePayloadInt(packet, "payload.viewport.xStart");
          viewport.xEnd = Asn1Helpers::asn1DecodePayloadInt(packet, "payload.viewport.xEnd");
          viewport.yStart = Asn1Helpers::asn1DecodePayloadInt(packet, "payload.viewport.yStart");
          viewport.yEnd = Asn1Helpers::asn1DecodePayloadInt(packet, "payload.viewport.yEnd");
          std::vector<uint8_t> buffer = Asn1Helpers::asn1DecodePayloadBuffer(packet, "payload.viewport.payload");
          viewport.payload = bufferToTable(buffer.data(), buffer.size());

          data.payload = viewport;
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

  std::shared_ptr<arrow::Buffer> Networking::tableToBuffer(const std::shared_ptr<arrow::Table>& table) {
      // Create output buffer with table structure
      std::shared_ptr<arrow::io::BufferOutputStream> outputStream = *arrow::io::BufferOutputStream::Create();
      std::shared_ptr<arrow::ipc::RecordBatchWriter> streamWriter = *arrow::ipc::MakeStreamWriter(outputStream, table->schema());
      arrow::Status status = streamWriter->WriteTable(*table);

      if (!status.ok()) {
        std::wcout << "Something went wrong while writing the structure!" << std::endl;
      }
            
      streamWriter->Close();
      arrow::Result<std::shared_ptr<arrow::Buffer>> buffer = outputStream->Finish();

      if (!buffer.ok()) {
        std::wcout << "Something went wrong while converting table to buffer!" << std::endl;
      }

      return *buffer;
  }

  std::shared_ptr<arrow::Table> Networking::bufferToTable(const uint8_t* rawData, int64_t dataSize) {
    std::shared_ptr<arrow::Buffer> buffer = arrow::Buffer::Wrap(rawData, dataSize);
    std::shared_ptr<arrow::io::InputStream> inputStream = std::make_shared<arrow::io::BufferReader>(buffer);
    std::shared_ptr<arrow::ipc::RecordBatchStreamReader> stream_reader = *arrow::ipc::RecordBatchStreamReader::Open(inputStream);
    std::shared_ptr<arrow::Table> table = *stream_reader->ToTable();
    return table;
  }

  void Networking::disconnect() {}
}
