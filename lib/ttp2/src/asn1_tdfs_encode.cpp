#include "../include/asn1_tdfs_encode.h"
#include "../include/asn1_helpers.h"

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
  }
}
