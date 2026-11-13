#ifndef ASN1_ENCODE_H
#define ASN1_ENCODE_H

#include <vector>

#include "../packet_types.h"

extern "C" {
#include <libtasn1.h>
extern const asn1_static_node packets_asn1_tab[];
}

namespace ttp2::asn1::encode {
  class Encode {
    public:
      static std::vector<unsigned char> encode(ttp2::Packet::payloadVariants payload, int id);

    private:
      static asn1_node encodeStandard(asn1_node packet, ttp2::Packet::Standard standard);
      static asn1_node encodeFile(asn1_node packet, ttp2::Packet::File file);
      static asn1_node encodeViewportRequest(asn1_node packet, ttp2::Packet::ViewportRequest viewportRequest);
      static asn1_node encodeViewport(asn1_node packet, ttp2::Packet::Viewport viewport);
      static asn1_node encodeTqlQuery(asn1_node packet, ttp2::Packet::TqlQuery tqlQuery);
      static asn1_node encodeError(asn1_node packet, ttp2::Packet::Error error);
      static asn1_node encodeUniversal(asn1_node packet, ttp2::Packet::Universal universal);
  };
}

#endif
