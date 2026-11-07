#include "../../include/helpers/asn1_decode.h"
#include "../../include/helpers/asn1_helpers.h"
#include "../../include/helpers/asn1_tdfs_decode.h"
#include "../../include/tdfs_packet_types.h"

#include <libtasn1.h>
#include <stdexcept>

#include <iostream>

namespace ttp2::asn1::decode {
  Decode::Decode() {
    char errorDescription[ASN1_MAX_ERROR_DESCRIPTION_SIZE];

    if (asn1_array2tree(packets_asn1_tab, &definition, errorDescription) !=
        ASN1_SUCCESS) {
      std::cout << "DECODE FAILS HERE!!!!" << std::endl;
      throw std::runtime_error(errorDescription);
    }
  }
  
  ttp2::Packet::Packet Decode::decode(std::vector<char> derBuffer) {
    char errorDescription[ASN1_MAX_ERROR_DESCRIPTION_SIZE];
    ttp2::Packet::Packet data;
    asn1_node packet = nullptr;

    asn1_create_element(definition, "Packets.Packet", &packet);

    if (asn1_der_decoding(&packet, derBuffer.data(), derBuffer.size(), errorDescription) ==
        ASN1_SUCCESS) {
      data.id = ttp2::Asn1Helpers::asn1DecodePayloadInt(packet, "id");

      char typeName[64];
      int branchSize = sizeof(typeName);
      int status = asn1_read_value(packet, "payload", typeName, &branchSize);
      std::string typeNameString = typeName;
      if (typeNameString == "standard") {
        data.payload = decodeStandard(packet);
      } else if (typeNameString == "file") {
        data.payload = decodeFile(packet);
      } else if (typeNameString == "viewportRequest") {
        data.payload = decodeViewportRequest(packet);
      } else if (typeNameString == "viewport") {
        data.payload = decodeViewport(packet);
      } else if (typeNameString == "tqlQuery") {
        data.payload = decodeTqlQuery(packet);
      } else if (typeNameString == "error") {
        data.payload = decodeError(packet);
      } else if (typeNameString == "universal") {
        data.payload = decodeUniversal(packet);
      } else if (typeNameString == "ls") {
        data.payload = tdfs::decodeLs(packet);
      } else if (typeNameString == "lsSolution") {
        data.payload = tdfs::decodeLsSolution(packet);
      } else {
        throw std::invalid_argument("Error decoding payload: Unknown type!");
      }
    } else {
      throw std::invalid_argument("Error decoding ASN1");
    }

    asn1_delete_structure(&packet);

    return data;
  }

  ttp2::Packet::Standard Decode::decodeStandard(asn1_node packet) {
    ttp2::Packet::Standard standard;
    standard.payload = ttp2::Asn1Helpers::asn1DecodePayloadString(packet, "payload.standard.payload");
    return standard;
  }
  
  ttp2::Packet::File Decode::decodeFile(asn1_node packet) {
    ttp2::Packet::File file;
    file.filePath = ttp2::Asn1Helpers::asn1DecodePayloadString(packet, "payload.file.filePath");
    file.start = ttp2::Asn1Helpers::asn1DecodePayloadInt(packet, "payload.file.start");
    file.end = ttp2::Asn1Helpers::asn1DecodePayloadInt(packet, "payload.file.end");

    std::vector<uint8_t> buffer = ttp2::Asn1Helpers::asn1DecodePayloadBuffer(packet, "payload.file.payload");
    // const uint8_t* bufferConst = buffer.data();
    file.payload = ttp2::Asn1Helpers::bufferToTable(buffer.data(), buffer.size());

    return file;
  }
  
  ttp2::Packet::ViewportRequest Decode::decodeViewportRequest(asn1_node packet) {
    ttp2::Packet::ViewportRequest viewportRequest;

    viewportRequest.xStart = ttp2::Asn1Helpers::asn1DecodePayloadInt(packet, "payload.viewportRequest.xStart");
    viewportRequest.xEnd = ttp2::Asn1Helpers::asn1DecodePayloadInt(packet, "payload.viewportRequest.xEnd");
    viewportRequest.yStart = ttp2::Asn1Helpers::asn1DecodePayloadInt(packet, "payload.viewportRequest.yStart");
    viewportRequest.yEnd = ttp2::Asn1Helpers::asn1DecodePayloadInt(packet, "payload.viewportRequest.yEnd");

    return viewportRequest;
  }
  
  ttp2::Packet::Viewport Decode::decodeViewport(asn1_node packet) {
    ttp2::Packet::Viewport viewport;
    viewport.xStart = ttp2::Asn1Helpers::asn1DecodePayloadInt(packet, "payload.viewport.xStart");
    viewport.xEnd = ttp2::Asn1Helpers::asn1DecodePayloadInt(packet, "payload.viewport.xEnd");
    viewport.yStart = ttp2::Asn1Helpers::asn1DecodePayloadInt(packet, "payload.viewport.yStart");
    viewport.yEnd = ttp2::Asn1Helpers::asn1DecodePayloadInt(packet, "payload.viewport.yEnd");
    std::vector<uint8_t> buffer = ttp2::Asn1Helpers::asn1DecodePayloadBuffer(packet, "payload.viewport.payload");
    viewport.payload = ttp2::Asn1Helpers::bufferToTable(buffer.data(), buffer.size());

    return viewport;
  }
  
  ttp2::Packet::TqlQuery Decode::decodeTqlQuery(asn1_node packet) {
    ttp2::Packet::TqlQuery tqlQuery;
    tqlQuery.query = ttp2::Asn1Helpers::asn1DecodePayloadString(packet, "payload.tqlQuery.query");
    return tqlQuery;
  }

  ttp2::Packet::Error Decode::decodeError(asn1_node packet) {
    ttp2::Packet::Error error;
    error.code = ttp2::Asn1Helpers::asn1DecodePayloadInt(packet, "payload.error.code");
    error.message = ttp2::Asn1Helpers::asn1DecodePayloadString(packet, "payload.error.message");
    return error;
  }

  ttp2::Packet::Universal Decode::decodeUniversal(asn1_node packet) {
    ttp2::Packet::Universal universal;
    universal.type = ttp2::Asn1Helpers::asn1DecodePayloadInt(packet, "payload.universal.type");
    universal.bytes = ttp2::Asn1Helpers::asn1DecodePayloadBuffer(packet, "payload.universal.bytes");
    return universal;
  }
}
