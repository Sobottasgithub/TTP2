#include "../../include/helpers/asn1_tdfs_encode.h"
#include "../../include/helpers/asn1_helpers.h"

namespace ttp2::asn1::encode {
  namespace tdfs {
    asn1_node encodeLs(asn1_node packet, ttp2::Packet::tdfs::Ls ls) {
      // Write structure
      int status = asn1_write_value(packet, "payload", "ls", 0);

      if (status != ASN1_SUCCESS) {
        throw std::invalid_argument("ASN1 set payload as ls failed!");
      }

      // Write contents
      return ttp2::Asn1Helpers::asn1EncodePayload(ls.directory, packet, "payload.ls.directory");
    }
    
    asn1_node encodeLsSolution(asn1_node packet, ttp2::Packet::tdfs::LsSolution lsSolution) {
      // Write structure
      int status = asn1_write_value(packet, "payload", "lsSolution", 0);

      if (status != ASN1_SUCCESS) {
        throw std::invalid_argument("ASN1 set payload as lsSolution failed!");
      }

      // Write contents
      return ttp2::Asn1Helpers::asn1EncodePayload(lsSolution.content, packet, "payload.lsSolution.directoryItems");
    }

    asn1_node encodeCopyFromLocal(asn1_node packet, ttp2::Packet::tdfs::CopyFromLocal copyFromLocal) {
      // Write structure
      int status = asn1_write_value(packet, "payload", "copyFromLocal", 0);

      if (status != ASN1_SUCCESS) {
        throw std::invalid_argument("ASN1 set payload as tdfs CopyFromLocal failed!");
      }

      // Write contents
      std::string filePathString = copyFromLocal.filePath;
      packet = ttp2::Asn1Helpers::asn1EncodePayload(filePathString, packet, "payload.copyFromLocal.filePath");

      int start = copyFromLocal.start;
      packet = ttp2::Asn1Helpers::asn1EncodePayload(start, packet, "payload.copyFromLocal.start");

      int end = copyFromLocal.end;
      packet = ttp2::Asn1Helpers::asn1EncodePayload(end, packet, "payload.copyFromLocal.end");

      std::shared_ptr<arrow::Table> table = copyFromLocal.payload;
      std::shared_ptr<arrow::Buffer> buffer = ttp2::Asn1Helpers::tableToBuffer(table);
      if (buffer->size() > 0) {
        packet = ttp2::Asn1Helpers::asn1EncodePayload(buffer->data(), buffer->size(), packet, "payload.copyFromLocal.payload");
      }

      return packet;
    }
  }
}
