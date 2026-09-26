#ifndef ASN1_TDFS_DECODE_H
#define ASN1_TDFS_DECODE_H

#include "tdfs_packet_types.h"

extern "C" {
#include <libtasn1.h>
extern const asn1_static_node packets_asn1_tab[];
}

namespace ttp2::asn1::decode {
  namespace tdfs {
    ttp2::Packet::tdfs::Ls decodeLs(asn1_node packet);
    ttp2::Packet::tdfs::LsSolution decodeLsSolution(asn1_node packet);
  }
}

#endif
