#include "../include/asn1_helpers.h"

#include <tablog_registry.h>
#include <tablog.h>

#include <iostream>
#include <string>
#include <cstring>
#include <vector>
#include <stdint.h>

extern "C" {
#include <libtasn1.h>
extern const asn1_static_node packets_asn1_tab[];
}

using namespace std;

namespace ttp2 {
    asn1_node Asn1Helpers::asn1EncodePayload(std::string payload, asn1_node packet, const char* asn1Key) {
    const char *standardPayload = payload.c_str();

    int status = asn1_write_value(packet, asn1Key,
                              standardPayload, std::strlen(standardPayload));

    if (status != ASN1_SUCCESS) {
      std::string asn1KeyString = asn1Key;
      tablog::TablogRegistry::getInstance().get("TTP2")->log(tablog::ERROR, "ASN1 set " + asn1KeyString + " failed!");
    }

    return packet;
  }

  asn1_node Asn1Helpers::asn1EncodePayload(int payload, asn1_node packet, const char* asn1Key) {
    std::string payloadString = std::to_string(payload);
    int status = asn1_write_value(packet, asn1Key,
                              payloadString.c_str(), 0);

    if (status != ASN1_SUCCESS) {
      std::string asn1KeyString = asn1Key;
      tablog::TablogRegistry::getInstance().get("TTP2")->log(tablog::ERROR, "ASN1 set " + asn1KeyString + " failed!");
    }

    return packet;
  }

  asn1_node Asn1Helpers::asn1EncodePayload(const uint8_t* buffer, int size, asn1_node packet, const char* asn1Key) {
    void* targetBuffer = const_cast<uint8_t*>(buffer);

    int status = asn1_write_value(packet, asn1Key, targetBuffer, size);

    if (status != ASN1_SUCCESS) {
      std::string asn1KeyString = asn1Key;
      tablog::TablogRegistry::getInstance().get("TTP2")->log(tablog::ERROR, "ASN1 set " + asn1KeyString + " failed!");
    }

    return packet;

  }

  std::string Asn1Helpers::asn1DecodePayloadString(asn1_node packet, const char* asn1Key) {
    int payloadLen = 0;
    std::vector<char> payloadStr(payloadLen);
    asn1_read_value(packet, asn1Key, nullptr, &payloadLen);
    if (payloadLen > 0) {
      payloadStr.resize(payloadLen);
      asn1_read_value(packet, asn1Key, payloadStr.data(),
                      &payloadLen);
    }
    std::string result = "";
    result.assign(payloadStr.data(), payloadLen);
    return result;
  }

  int Asn1Helpers::asn1DecodePayloadInt(asn1_node packet, const char* asn1Key) {
    int payloadLen = 0;
    std::vector<char> payloadBytes(payloadLen);
    asn1_read_value(packet, asn1Key, nullptr, &payloadLen);
    if (payloadLen > 0) {
      payloadBytes.resize(payloadLen);
      asn1_read_value(packet, asn1Key, payloadBytes.data(),
                      &payloadLen);
    }
    return bytesToInt(payloadBytes, payloadLen);
  }

  int Asn1Helpers::bytesToInt(std::vector<char> bytes, int size) {
    int result = 0;
    for (int index = 0; index < size; index++)
    {
        result <<= 8;
        result |= (bytes[index] & 0xFF);
    }
    return result;
  }

  std::vector<uint8_t> Asn1Helpers::asn1DecodePayloadBuffer(asn1_node packet, const char* asn1Key) {
    int payloadLen = 0;
    int status = asn1_read_value(packet, asn1Key, nullptr, &payloadLen);
    std::vector<uint8_t> buffer(payloadLen);

    if (status == ASN1_MEM_ERROR && payloadLen > 0) {
        status = asn1_read_value(packet, asn1Key, buffer.data(), &payloadLen);
        return buffer;
    }
    return buffer;
  }
}
