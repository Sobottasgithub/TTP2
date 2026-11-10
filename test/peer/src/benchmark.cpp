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
