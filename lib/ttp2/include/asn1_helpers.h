#ifndef ASN1_HELPERS_H
#define ASN1_HELPERS_H

#include <tablog.h>

#include <string>
#include <vector>
#include <stdint.h>
#include <memory>
#include <arrow/buffer.h>
#include <arrow/table.h>

extern "C" {
#include <libtasn1.h>
extern const asn1_static_node packets_asn1_tab[];
}

namespace ttp2 {
  class Asn1Helpers {
    public:
      static asn1_node asn1EncodePayload(std::string payload, asn1_node packet, const char* asn1Key);
      static asn1_node asn1EncodePayload(int payload, asn1_node packet, const char* asn1Key);
      static asn1_node asn1EncodePayload(const uint8_t* buffer, int size, asn1_node packet, const char* asn1Key);

      static std::string asn1DecodePayloadString(asn1_node packet, const char* asn1Key);
      static int asn1DecodePayloadInt(asn1_node packet, const char* asn1Key);
      static std::vector<uint8_t> asn1DecodePayloadBuffer(asn1_node packet, const char* asn1Key);

      static std::shared_ptr<arrow::Buffer> tableToBuffer(const std::shared_ptr<arrow::Table>& table);
    private:
      static int bytesToInt(std::vector<char> bytes, int size);
  };
}

#endif
