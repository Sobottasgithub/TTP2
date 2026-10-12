#include "cli.h"

#include <iostream>
#include <regex>

namespace test {
  Cli::Cli() {
    while (true) {
      int option = requestInt("(1) connect\n(2) send\n(3) receive\nchoose option:");

      switch (option) {
        case 1:
          continue;
        case 2:
          continue;
        case 3:
          continue;
        default:
          std::cout << "Unknown option!" << std::endl;
      }
    }
  }


  bool Cli::isNumeric(const std::string& string) {
    static const std::regex numberRegex(
        R"(^[-+]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][-+]?\d+)?$)"
    );
    return std::regex_match(string, numberRegex);
  }
  
  std::string Cli::requestString(const std::string& message) {
    std::string userInput;
    std::cout << message << std::flush;
    std::getline(std::cin, userInput);
    return userInput;
  }
  
  int Cli::requestInt(const std::string& message) {
    std::string userInput;

    while (true) {
        std::cout << message << std::flush;
        std::getline(std::cin, userInput);

        if (isNumeric(userInput)) {
            return std::stoi(userInput);
        }
        std::cout << "Invalid input! Try again" << std::endl;
    }
  }
}
