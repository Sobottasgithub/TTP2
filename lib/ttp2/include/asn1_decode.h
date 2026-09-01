#ifndef ASN1_DECODE_H
#define ASN1_DECODE_H

#include <vector>

#include "networking.h"

extern "C" {
#include <libtasn1.h>
extern const asn1_static_node packets_asn1_tab[];
}

namespace ttp2::asn1::decode {
  ttp2::Networking::Packet decode(std::vector<char> derBuffer);

  ttp2::Networking::Standard decodeStandard(asn1_node packet);
  ttp2::Networking::File decodeFile(asn1_node packet);
  ttp2::Networking::ViewportRequest decodeViewportRequest(asn1_node packet);
  ttp2::Networking::Viewport decodeViewport(asn1_node packet);
  ttp2::Networking::TqlQuery decodeTqlQuery(asn1_node packet);
}

#endif
