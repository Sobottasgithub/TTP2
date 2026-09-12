#include "../include/asn1_encode.h"
#include "../include/asn1_helpers.h"
#include <stdexcept>
#include <variant>

namespace ttp2::asn1::encode {
  std::vector<unsigned char> encode(ttp2::Packet::payloadVariants payload, int id) {
    asn1_node definitions = nullptr;
    asn1_node packet = nullptr;
    char errorDescription[ASN1_MAX_ERROR_DESCRIPTION_SIZE];

    if (asn1_array2tree(packets_asn1_tab, &definitions, errorDescription) !=
        ASN1_SUCCESS) {
      std::string errorDescriptionString = errorDescription;
      throw std::invalid_argument("Error in sendMessage when loading asn1: " + errorDescriptionString);
    }

    asn1_create_element(definitions, "Packets.Packet", &packet);
    packet = ttp2::Asn1Helpers::asn1EncodePayload(id, packet, "id");

    if (std::holds_alternative<ttp2::Packet::Standard>(payload)) {
      packet = encodeStandard(packet, std::get<ttp2::Packet::Standard>(payload));
    } else if (std::holds_alternative<ttp2::Packet::File>(payload)) {
      packet = encodeFile(packet, std::get<ttp2::Packet::File>(payload));
    } else if (std::holds_alternative<ttp2::Packet::ViewportRequest>(payload)) {
      packet = encodeViewportRequest(packet, std::get<ttp2::Packet::ViewportRequest>(payload));
    } else if (std::holds_alternative<ttp2::Packet::Viewport>(payload)) {
      packet = encodeViewport(packet, std::get<ttp2::Packet::Viewport>(payload));
    } else if (std::holds_alternative<ttp2::Packet::TqlQuery>(payload)) {
      packet = encodeTqlQuery(packet, std::get<ttp2::Packet::TqlQuery>(payload));
    } else if (std::holds_alternative<ttp2::Packet::Error>(payload)) {
      packet = encodeError(packet, std::get<ttp2::Packet::Error>(payload));
    }
    
    int derLen = 0;
    asn1_der_coding(packet, "", nullptr, &derLen, nullptr);
    std::vector<unsigned char> buffer(derLen);
    if (asn1_der_coding(packet, "", buffer.data(), &derLen, errorDescription) !=
        ASN1_SUCCESS) {
      std::string errorDescriptionString = errorDescription;
      throw std::invalid_argument("Error while encoding packet: " + errorDescriptionString);
      abort();
    }

    asn1_delete_structure(&packet);
    asn1_delete_structure(&definitions);

    return buffer;
  }

  asn1_node encodeStandard(asn1_node packet, ttp2::Packet::Standard standard) {
    // Write structure
    int status = asn1_write_value(packet, "payload", "standard", 0);

    if (status != ASN1_SUCCESS) {
      throw std::invalid_argument("ASN1 set payload as standard failed!");
    }

    // Write contents
    return ttp2::Asn1Helpers::asn1EncodePayload(standard.payload, packet, "payload.standard.payload");
  }
  
  asn1_node encodeFile(asn1_node packet, ttp2::Packet::File file) {
    // Write structure
    int status = asn1_write_value(packet, "payload", "file", 0);

    if (status != ASN1_SUCCESS) {
      throw std::invalid_argument("ASN1 set payload as file failed!");
    }

    // Write contents
    std::string filePathString = file.filePath;
    packet = ttp2::Asn1Helpers::asn1EncodePayload(filePathString, packet, "payload.file.filePath");

    int start = file.start;
    packet = ttp2::Asn1Helpers::asn1EncodePayload(start, packet, "payload.file.start");

    int end = file.end;
    packet = ttp2::Asn1Helpers::asn1EncodePayload(end, packet, "payload.file.end");

    std::shared_ptr<arrow::Table> table = file.payload;
    std::shared_ptr<arrow::Buffer> buffer = ttp2::Asn1Helpers::tableToBuffer(table);
    if (buffer->size() > 0) {
      packet = ttp2::Asn1Helpers::asn1EncodePayload(buffer->data(), buffer->size(), packet, "payload.file.payload");
    }

    return packet;
  }
  
  asn1_node encodeViewportRequest(asn1_node packet, ttp2::Packet::ViewportRequest viewportRequest) {
    // Write structure
    int status = asn1_write_value(packet, "payload", "viewportRequest", 0);

    if (status != ASN1_SUCCESS) {
      throw std::invalid_argument("ASN1 set payload as viewport request failed!");
    }

    // Write content
    // X
    int xStart = viewportRequest.xStart;
    packet = ttp2::Asn1Helpers::asn1EncodePayload(xStart, packet, "payload.viewportRequest.xStart");
    int xEnd = viewportRequest.xEnd;
    packet = ttp2::Asn1Helpers::asn1EncodePayload(xEnd, packet, "payload.viewportRequest.xEnd");

    // Y
    int yStart = viewportRequest.yStart;
    packet = ttp2::Asn1Helpers::asn1EncodePayload(yStart, packet, "payload.viewportRequest.yStart");
    int yEnd = viewportRequest.yEnd;
    packet = ttp2::Asn1Helpers::asn1EncodePayload(yEnd, packet, "payload.viewportRequest.yEnd");
    return packet;
  }
  
  asn1_node encodeViewport(asn1_node packet, ttp2::Packet::Viewport viewport) {
    // Write structure
    int status = asn1_write_value(packet, "payload", "viewport", 0);

    if (status != ASN1_SUCCESS) {
      throw std::invalid_argument("ASN1 set payload as viewport failed!");
    }

    // Write content
    // X
    int xStart = viewport.xStart;
    packet = ttp2::Asn1Helpers::asn1EncodePayload(xStart, packet, "payload.viewport.xStart");
    int xEnd = viewport.xEnd;
    packet = ttp2::Asn1Helpers::asn1EncodePayload(xEnd, packet, "payload.viewport.xEnd");

    // Y
    int yStart = viewport.yStart;
    packet = ttp2::Asn1Helpers::asn1EncodePayload(yStart, packet, "payload.viewport.yStart");
    int yEnd = viewport.yEnd;
    packet = ttp2::Asn1Helpers::asn1EncodePayload(yEnd, packet, "payload.viewport.yEnd");

    std::shared_ptr<arrow::Table> table = viewport.payload;
    std::shared_ptr<arrow::Buffer> buffer = ttp2::Asn1Helpers::tableToBuffer(table);
    packet = ttp2::Asn1Helpers::asn1EncodePayload(buffer->data(), buffer->size(), packet, "payload.viewport.payload");
    return packet;
  }
  
  asn1_node encodeTqlQuery(asn1_node packet, ttp2::Packet::TqlQuery tqlQuery) {
    // Write structure
    int status = asn1_write_value(packet, "payload", "tqlQuery", 0);
    if (status != ASN1_SUCCESS) {
      throw std::invalid_argument("ASN1 set payload as tql query failed!");
    }

    // Write content
    std::string query = tqlQuery.query;
    packet = ttp2::Asn1Helpers::asn1EncodePayload(query, packet, "payload.tqlQuery.query");
    return packet;
  }

  asn1_node encodeError(asn1_node packet, ttp2::Packet::Error error) {
    // Write structure
    int status = asn1_write_value(packet, "payload", "error", 0);
    if (status != ASN1_SUCCESS) {
      throw std::invalid_argument("ASN1 set payload as error failed!");
    }

    // Write content
    int errorCode = error.code;
    packet = ttp2::Asn1Helpers::asn1EncodePayload(errorCode, packet, "payload.error.code");

    std::string errorMessage = error.message;
    packet = ttp2::Asn1Helpers::asn1EncodePayload(errorMessage, packet, "payload.error.message");
    
    return packet;
  }
}
