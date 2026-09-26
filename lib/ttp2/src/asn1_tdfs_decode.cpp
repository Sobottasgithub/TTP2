#include "../include/asn1_tdfs_decode.h"
#include "../include/asn1_helpers.h"

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
  }
}
