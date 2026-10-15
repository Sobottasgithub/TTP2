#include "cli.h"

#include "peer_session_controller.h"

#include <iostream>
#include <regex>
#include <thread>

namespace test {
  Cli::Cli() {
    auto peerSessionController = std::make_shared<ttp2::PeerSessionController>();
    std::thread networkingSession([peerSessionController]() {
      peerSessionController->networkingSession();
    });
    
    while (true) {
      int option = requestInt("(1) send\n(2) receive\nchoose option: ");

      switch (option) {
        case 1:
          continue;
        case 2:
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
