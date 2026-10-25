#include "cli.h"

#include "peer_session_controller.h"
#include "packet_types.h"

#include <iostream>
#include <regex>
#include <thread>

namespace test {
  Cli::Cli() {
    std::string ip = requestString("Server ip: ");
    int port = requestInt("Server port: ");
    
    auto peerSessionController = std::make_shared<ttp2::PeerSessionController>(ip.c_str(), port);
    std::thread networkingSession([peerSessionController]() {
      peerSessionController->networkingSession();
    });
    
    while (peerSessionController->isConnected()) {
      int option = requestInt("(1) send\n(2) receive\n(3) exit\nchoose option: ");
      
      std::string receiverIp = "";
      int receiverPort = 0;
      std::string message = "";
      ttp2::PeerSessionController::AddressedPacket addressPacket; 
      ttp2::Packet::Packet packet;
      ttp2::Packet::Standard standardPacket;

      switch (option) {
        case 1:
          receiverIp = requestString("Server ip: ");
          receiverPort = requestInt("Server port: ");
          message = requestString("Message: ");

          standardPacket.payload = message;
          packet.payload = standardPacket;
          addressPacket.ip = receiverIp.c_str();
          addressPacket.port = receiverPort;
          addressPacket.packet = packet;

          peerSessionController->sendPacket(addressPacket);
          continue;
        case 2:
          while (peerSessionController->hasPacket()) {
            ttp2::PeerSessionController::AddressedPacket receivedPacket = peerSessionController->popReceivedPacket();
            std::cout << "-------------Start-------------" << std::endl;
            std::cout << "IP: " << receivedPacket.ip << std::endl;
            std::cout << "ID: " << receivedPacket.packet.id << std::endl;
            if (std::holds_alternative<ttp2::Packet::Standard>(receivedPacket.packet.payload)) {
              ttp2::Packet::Standard standard = std::get<ttp2::Packet::Standard>(receivedPacket.packet.payload);
              std::cout << "Payload: " << standard.payload << std::endl;
            }
            std::cout << "------------- End -------------" << std::endl;
          }
          continue;
        case 3:
          peerSessionController->disconnect();
          continue;
        default:
          std::cout << "Unknown option!" << std::endl;
      }
    }

    if (networkingSession.joinable()) {
      networkingSession.join();
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
