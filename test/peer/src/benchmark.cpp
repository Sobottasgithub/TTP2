#include "benchmark.h"
#include "helpers.h"

#include <iostream>
#include <thread>

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

    std::thread sendSessionThread([this, receiverIp, receiverPort, payloadSize] {
      this->sendBenchmarkPackets(receiverIp.c_str(), receiverPort, payloadSize);
    });

    const int duration = 10;
    std::thread timerSessionThread([this, duration] {
      this->timerSession(duration);
    });

    while (isConnected()) {
      while(peerSessionController->hasPacket()) {
        peerSessionController->popReceivedPacket();
      }
    }

    if (sendSessionThread.joinable()) {
      sendSessionThread.join();
    }

    if (timerSessionThread.joinable()) {
      timerSessionThread.join();
    }

    std::cout << "Cleaning up..." << std::endl;
    // INFO: Cleanup left over packets
    while(peerSessionController->hasPacket()) {
      peerSessionController->popReceivedPacket();
    }

    return;
  }

  void Benchmark::sendBenchmarkPackets(const char* ip, const int port, const int payloadSize) {
    ttp2::Packet::Standard standardPacket;
    const std::string payload(std::max(0, payloadSize), '0');
    standardPacket.payload = payload;
    ttp2::Packet::Packet packet;
    packet.payload = standardPacket;
    ttp2::PeerSessionController::AddressedPacket addressPacket;
    addressPacket.ip = ip;
    addressPacket.port = port;
    addressPacket.packet = packet;

    while(isConnected()) {
      peerSessionController->sendPacket(addressPacket);
    }
  }

  void Benchmark::timerSession(const int duration) {
    auto start = std::chrono::steady_clock::now();
    auto deltaStart = start;
    
    while (true) {
      auto now = std::chrono::steady_clock::now();
      auto elapsedTotal = std::chrono::duration_cast<std::chrono::seconds>(now - start).count();
      if (elapsedTotal >= duration) {
        disconnect();
        break;
      }
      
      auto deltaElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - deltaStart).count();
      if (deltaElapsed >= 1000) {
        // TODO: take pps in main thread
      }
    }
  }

  bool Benchmark::isConnected() {
    std::lock_guard<std::mutex> lock(this->connectedMutex);
    return this->connected;
  }

  void Benchmark::disconnect() {
    std::lock_guard<std::mutex> lock(this->connectedMutex);
    this->connected = false;
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
