#include "../include/asn1_decode.h"
#include "../include/asn1_helpers.h"
#include <stdexcept>

namespace ttp2::asn1::decode {
  ttp2::Networking::Packet decode(std::vector<char> derBuffer) {
    ttp2::Networking::Packet data;

    asn1_node definitions = nullptr;
    asn1_node packet = nullptr;
    char errorDescription[ASN1_MAX_ERROR_DESCRIPTION_SIZE];

    if (asn1_array2tree(packets_asn1_tab, &definitions, errorDescription) !=
        ASN1_SUCCESS) {
      return data;
    }

    asn1_create_element(definitions, "Packets.Packet", &packet);

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
        data.payload = decodeViewportRequest(packet);
      } else if (typeNameString == "tqlQuery") {
        data.payload = decodeTqlQuery(packet);
      } else {
        throw std::invalid_argument("Error decoding payload: Unknown type!");
      }
    } else {
      throw std::invalid_argument("Error decoding ASN1");
    }

    asn1_delete_structure(&packet);
    asn1_delete_structure(&definitions);

    return data;
  }

  ttp2::Networking::Standard decodeStandard(asn1_node packet) {
    ttp2::Networking::Standard standard;
    standard.payload = ttp2::Asn1Helpers::asn1DecodePayloadString(packet, "payload.standard.payload");
    return standard;
  }
  
  ttp2::Networking::File decodeFile(asn1_node packet) {
    ttp2::Networking::File file;
    file.filePath = ttp2::Asn1Helpers::asn1DecodePayloadString(packet, "payload.file.filePath");
    file.start = ttp2::Asn1Helpers::asn1DecodePayloadInt(packet, "payload.file.start");
    file.end = ttp2::Asn1Helpers::asn1DecodePayloadInt(packet, "payload.file.end");

    std::vector<uint8_t> buffer = ttp2::Asn1Helpers::asn1DecodePayloadBuffer(packet, "payload.file.payload");
    // const uint8_t* bufferConst = buffer.data();
    file.payload = ttp2::Asn1Helpers::bufferToTable(buffer.data(), buffer.size());

    return file;
  }
  
  ttp2::Networking::ViewportRequest decodeViewportRequest(asn1_node packet) {
    ttp2::Networking::ViewportRequest viewportRequest;

    viewportRequest.xStart = ttp2::Asn1Helpers::asn1DecodePayloadInt(packet, "payload.viewportRequest.xStart");
    viewportRequest.xEnd = ttp2::Asn1Helpers::asn1DecodePayloadInt(packet, "payload.viewportRequest.xEnd");
    viewportRequest.yStart = ttp2::Asn1Helpers::asn1DecodePayloadInt(packet, "payload.viewportRequest.yStart");
    viewportRequest.yEnd = ttp2::Asn1Helpers::asn1DecodePayloadInt(packet, "payload.viewportRequest.yEnd");

    return viewportRequest;
  }
  
  ttp2::Networking::Viewport decodeViewport(asn1_node packet) {
    ttp2::Networking::Viewport viewport;
    viewport.xStart = ttp2::Asn1Helpers::asn1DecodePayloadInt(packet, "payload.viewport.xStart");
    viewport.xEnd = ttp2::Asn1Helpers::asn1DecodePayloadInt(packet, "payload.viewport.xEnd");
    viewport.yStart = ttp2::Asn1Helpers::asn1DecodePayloadInt(packet, "payload.viewport.yStart");
    viewport.yEnd = ttp2::Asn1Helpers::asn1DecodePayloadInt(packet, "payload.viewport.yEnd");
    std::vector<uint8_t> buffer = ttp2::Asn1Helpers::asn1DecodePayloadBuffer(packet, "payload.viewport.payload");
    viewport.payload = ttp2::Asn1Helpers::bufferToTable(buffer.data(), buffer.size());

    return viewport;
  }
  
  ttp2::Networking::TqlQuery decodeTqlQuery(asn1_node packet) {
    ttp2::Networking::TqlQuery tqlQuery;
    tqlQuery.query = ttp2::Asn1Helpers::asn1DecodePayloadString(packet, "payload.tqlQuery.query");
    return tqlQuery;
  }
}
