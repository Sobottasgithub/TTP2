#ifndef CLI_H
#define CLI_H

#include <string>

namespace test {
  class Cli {
    public:
      Cli();

    private:
      bool isNumeric(const std::string& string);
      std::string requestString(const std::string& message);
      int requestInt(const std::string& message);

  };
}

#endif
