#ifndef TDFS_PACKET_TYPES_H
#define TDFS_PACKET_TYPES_H

#include <string>
#include <vector>

namespace ttp2::Packet {
  namespace tdfs {
    struct Ls {
      std::string directory;
    };

    struct LsSolution {
      std::vector<std::string> content;
    };
  }
}

#endif
