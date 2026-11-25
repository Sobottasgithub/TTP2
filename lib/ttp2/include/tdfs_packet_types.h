#ifndef TDFS_PACKET_TYPES_H
#define TDFS_PACKET_TYPES_H

#include <string>
#include <vector>
#include <memory>
#include <arrow/table.h>

namespace ttp2::Packet {
  namespace tdfs {
    struct Ls {
      std::string directory;
    };

    struct LsSolution {
      std::vector<std::string> content;
    };

    struct CopyFromLocal {
    	std::string filePath = "";
      unsigned int start = 0;
    	unsigned int end = 0;
    	std::shared_ptr<arrow::Table> payload = arrow::Table::Make(arrow::schema({}), std::vector<std::shared_ptr<arrow::Array>>{}, 0);
    };
  }
}

#endif
