#ifndef PACKET_TYPE_H
#define PACKET_TYPE_H

#include <string>
#include <arrow/table.h>
#include <memory>

namespace ttp2::Packet {
  struct Standard {
    std::string payload = "";
  };

  struct File {
  	std::string filePath = "";
    int start = -1;
  	int end = -1;
  	std::shared_ptr<arrow::Table> payload = arrow::Table::Make(arrow::schema({}), std::vector<std::shared_ptr<arrow::Array>>{}, 0);
  };

  struct ViewportRequest {
    int xStart = 0;
    int xEnd = 0;
    int yStart = 0;
    int yEnd = 0;
  };

  struct Viewport {
    int xStart = 0;
    int xEnd = 0;
    int yStart = 0;
    int yEnd = 0;
    std::shared_ptr<arrow::Table> payload = arrow::Table::Make(arrow::schema({}), std::vector<std::shared_ptr<arrow::Array>>{}, 0);
  };

  struct TqlQuery {
    std::string query = "";
  };

  typedef std::variant<Standard, File, ViewportRequest, Viewport, TqlQuery> payloadVariants;

  struct Packet {
    int id = -1;
    payloadVariants payload;  
  };
}

#endif
