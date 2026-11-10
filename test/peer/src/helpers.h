#ifndef HELPERS_H
#define HELPERS_H

#include <string>

namespace test {
  bool isNumeric(const std::string& string);
  std::string requestString(const std::string& message);
  int requestInt(const std::string& message);
}

#endif
