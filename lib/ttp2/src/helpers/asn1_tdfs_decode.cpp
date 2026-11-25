#include "../../include/helpers/asn1_tdfs_decode.h"
#include "../../include/helpers/asn1_helpers.h"

namespace ttp2::asn1::decode {
  namespace tdfs {
    ttp2::Packet::tdfs::Ls decodeLs(asn1_node packet) {
      ttp2::Packet::tdfs::Ls ls;
      ls.directory = ttp2::Asn1Helpers::asn1DecodePayloadString(packet, "payload.ls.directory");
      return ls;
    }
    
    ttp2::Packet::tdfs::LsSolution decodeLsSolution(asn1_node packet) {
      ttp2::Packet::tdfs::LsSolution lsSolution;
      lsSolution.content = ttp2::Asn1Helpers::asn1DecodePayloadVector(packet, "payload.lsSolution.directoryItems");
      return lsSolution;
    }

    ttp2::Packet::tdfs::CopyFromLocal decodeCopyFromLocal(asn1_node packet) {
      ttp2::Packet::tdfs::CopyFromLocal copyFromLocal;

      copyFromLocal.filePath = ttp2::Asn1Helpers::asn1DecodePayloadString(packet, "payload.copyFromLocal.filePath");
      copyFromLocal.start = ttp2::Asn1Helpers::asn1DecodePayloadInt(packet, "payload.copyFromLocal.start");
      copyFromLocal.end = ttp2::Asn1Helpers::asn1DecodePayloadInt(packet, "payload.copyFromLocal.end");

      std::vector<uint8_t> buffer = ttp2::Asn1Helpers::asn1DecodePayloadBuffer(packet, "payload.copyFromLocal.payload");
      copyFromLocal.payload = ttp2::Asn1Helpers::bufferToTable(buffer.data(), buffer.size());

      return copyFromLocal;
    }
  }
}
