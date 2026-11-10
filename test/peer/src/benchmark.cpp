#include "benchmark.h"
#include "helpers.h"

#include <iostream>

namespace test::benchmark {
  Benchmark::Benchmark(std::shared_ptr<ttp2::PeerSessionController> peerSessionController) {
    std::cout << "BENCHMARK" << std::endl;
    this->peerSessionController = peerSessionController;

    int benchmarkOption = requestInt("(0) exit\n(1) sender\n(2) returner\ntype:");

    switch(benchmarkOption) {
      case 0: {
        return;
      } case 1: {
        sendSession();
        return;
      } case 2: {
        returnSession();
        return;
      } default: {
        std::cout << "Unknown action.\nExit" << std::endl;
        return;
      }
    }
  }

  void Benchmark::sendSession() {
    std::string receiverIp = requestString("Server ip: ");
    int receiverPort = requestInt("Server port: ");
    int payloadSize = requestInt("Payload size (1024): ");

    ttp2::Packet::Standard standardPacket;
    const std::string payload(std::max(0, payloadSize), '0');
    standardPacket.payload = payload;
    ttp2::Packet::Packet packet;
    packet.payload = standardPacket;
    ttp2::PeerSessionController::AddressedPacket addressPacket;
    addressPacket.ip = receiverIp.c_str();
    addressPacket.port = receiverPort;
    addressPacket.packet = packet;

    int count = 10000;
    while (count > 0) {
      for (int index = 0; index < 10; index++) {
        peerSessionController->sendPacket(addressPacket);
      }

      while(peerSessionController->hasPacket()) {
        peerSessionController->popReceivedPacket();
        count--;
      }
    }
    
    return;
  }

  void Benchmark::returnSession() {
    while(true) {
      while (peerSessionController->hasPacket()) {
        ttp2::PeerSessionController::AddressedPacket receivedPacket = peerSessionController->popReceivedPacket();
        receivedPacket.port = receivedPacket.port - 1;
        peerSessionController->sendPacket(receivedPacket);
      }
    }
    return;
  }
}
