#ifndef PACKET_TYPE_H
#define PACKET_TYPE_H

#include "tdfs_packet_types.h"

#include <string>
#include <arrow/table.h>
#include <memory>

namespace ttp2::Packet {
  struct Standard {
    std::string payload = "";
  };

  struct File {
  	std::string filePath = "";
    unsigned int start = 0;
  	unsigned int end = 0;
  	std::shared_ptr<arrow::Table> payload = arrow::Table::Make(arrow::schema({}), std::vector<std::shared_ptr<arrow::Array>>{}, 0);
  };

  struct ViewportRequest {
    unsigned int xStart = 0;
    unsigned int xEnd = 0;
    unsigned int yStart = 0;
    unsigned int yEnd = 0;
  };

  struct Viewport {
    unsigned int xStart = 0;
    unsigned int xEnd = 0;
    unsigned int yStart = 0;
    unsigned int yEnd = 0;
    std::shared_ptr<arrow::Table> payload = arrow::Table::Make(arrow::schema({}), std::vector<std::shared_ptr<arrow::Array>>{}, 0);
  };

  struct TqlQuery {
    std::string query = "";
  };

  struct Error {
    unsigned int code = 0;
    std::string message = "";
  };

  struct Universal {
    int type;
    std::vector<uint8_t> bytes;
  };

  typedef std::variant<Standard, File, ViewportRequest, Viewport, TqlQuery, Error,
                       Universal, tdfs::Ls, tdfs::LsSolution> payloadVariants;

  struct Packet {
    int id = -1;
    payloadVariants payload;  
  };
}

#endif
