#ifndef ASN1_DECODE_H
#define ASN1_DECODE_H

#include <vector>

#include "packet_types.h"

extern "C" {
#include <libtasn1.h>
extern const asn1_static_node packets_asn1_tab[];
}

namespace ttp2::asn1::decode {
  ttp2::Packet::Packet decode(std::vector<char> derBuffer);

  ttp2::Packet::Standard decodeStandard(asn1_node packet);
  ttp2::Packet::File decodeFile(asn1_node packet);
  ttp2::Packet::ViewportRequest decodeViewportRequest(asn1_node packet);
  ttp2::Packet::Viewport decodeViewport(asn1_node packet);
  ttp2::Packet::TqlQuery decodeTqlQuery(asn1_node packet);
  ttp2::Packet::Error decodeError(asn1_node packet);
}

#endif
