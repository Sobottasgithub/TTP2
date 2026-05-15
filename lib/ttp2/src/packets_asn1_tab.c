#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <libtasn1.h>

const asn1_static_node packets_asn1_tab[] = {
  { "Packets", 536872976, NULL },
  { NULL, 1073741836, NULL },
  { "Standard", 1610612741, NULL },
  { "payload", 7, NULL },
  { "Packet", 536870917, NULL },
  { "id", 1073741827, NULL },
  { "method", 1073741827, NULL },
  { "payload", 536870930, NULL },
  { "standard", 536879106, "Standard"},
  { NULL, 2056, "0"},
  { NULL, 0, NULL }
};
