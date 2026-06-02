#if HAVE_CONFIG_H
# include "config.h"
#endif

#include <libtasn1.h>

const asn1_static_node packets_asn1_tab[] = {
  { "Packets", 536872976, NULL },
  { NULL, 1073741836, NULL },
  { "Standard", 1610612741, NULL },
  { "payload", 7, NULL },
  { "File", 1610612741, NULL },
  { "filePath", 1073741831, NULL },
  { "start", 1073741827, NULL },
  { "end", 1073741827, NULL },
  { "payload", 7, NULL },
  { "Packet", 536870917, NULL },
  { "id", 1073741827, NULL },
  { "payload", 536870930, NULL },
  { "standard", 1610620930, "Standard"},
  { NULL, 2056, "0"},
  { "file", 536879106, "File"},
  { NULL, 2056, "1"},
  { NULL, 0, NULL }
};
