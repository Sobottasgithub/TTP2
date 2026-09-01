#ifndef ASN1_ENCODE_H
#define ASN1_ENCODE_H

#include <vector>

#include "networking.h"

extern "C" {
#include <libtasn1.h>
extern const asn1_static_node packets_asn1_tab[];
}

namespace ttp2::asn1::encode {
  std::vector<unsigned char> encode(ttp2::Networking::payloadVariants payload, int id);

  asn1_node encodeStandard(asn1_node packet, ttp2::Networking::Standard standard);
  asn1_node encodeFile(asn1_node packet, ttp2::Networking::File file);
  asn1_node encodeViewportRequest(asn1_node packet, ttp2::Networking::ViewportRequest viewportRequest);
  asn1_node encodeViewport(asn1_node packet, ttp2::Networking::Viewport viewport);
  asn1_node encodeTqlQuery(asn1_node packet, ttp2::Networking::TqlQuery tqlQuery);
}

#endif
