#ifndef ASN1_TDFS_ENCODE_H
#define ASN1_TDFS_ENCODE_H

#include "tdfs_packet_types.h"

extern "C" {
#include <libtasn1.h>
extern const asn1_static_node packets_asn1_tab[];
}

namespace ttp2::asn1::encode {
  namespace tdfs {
    asn1_node encodeLs(asn1_node packet, ttp2::Packet::tdfs::Ls ls);
    asn1_node encodeLsSolution(asn1_node packet, ttp2::Packet::tdfs::LsSolution lsSolution);
  }
}

#endif
