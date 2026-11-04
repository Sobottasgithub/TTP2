#ifndef HELPERS_H
#define HELPERS_H

#include <string>
#include <memory>
#include <arrow/table.h>

namespace test {
  class Helpers {
    public:
     static bool isNumeric(const std::string& string);
     static std::string requestString(const std::string& message);
     static int requestInt(const std::string& message);
     static std::shared_ptr<arrow::Table> openCsvFile();
  };
}

#endif
