#include "benchmark.h"
#include "helpers.h"

#include <iostream>

#include <chrono>
#include <packet_types.h>
#include <vector>
#include <thread>

namespace test::benchmark {
  Benchmark::Benchmark(std::shared_ptr<ttp2::ClientSessionController> clientSessionController) {
    this->clientSessionController = clientSessionController;
    std::cout << "~~~~~~ ~~~~~~ TTP2 TCP Benchmark ~~~~~~ ~~~~~~" << std::endl;
    int payloadSize = test::Helpers::requestInt("(Int) payload size (1024): ");

    // TODO: Take user input
    const int duration = 60;
    std::vector<int> ppsCollection;
    ppsCollection.reserve(duration + 5);
    int totalPacketsReceived = 0;
    int deltaPacketsReceived = 0;

    std::thread sendSessionThread([this, payloadSize] {
        this->sendSession(payloadSize);
    });

    std::thread timerSessionThread([this, duration] {
      this->timerSession(duration);
    });

    while (isConnected()) {
        while (this->clientSessionController->hasResponse()) {
            auto responsePacket = this->clientSessionController->popResponse();
            totalPacketsReceived++;
            deltaPacketsReceived++;

            {
                std::lock_guard<std::mutex> lock(packetMutex);
                inFlight--;
            }
            packetReceivedSignal.notify_one();

            {
              std::lock_guard<std::mutex> lock(this->takeBenchmarkValuesMutex);
              if (this->takeBenchmarkValues) {
                this->takeBenchmarkValues = false;
                ppsCollection.emplace_back(deltaPacketsReceived);
                deltaPacketsReceived = 0;
              }
            }
        }
    }

    packetReceivedSignal.notify_all();

    if (sendSessionThread.joinable()) {
        sendSessionThread.join();
    }
    
    if (timerSessionThread.joinable()) {
      timerSessionThread.join();
    }
    std::cout << "Done joining threads" << std::endl;

    int peak = 0;
    for (int index = 0; index < ppsCollection.size(); index++) {
      if (ppsCollection.at(index) > peak) {
        peak = ppsCollection.at(index);
      }
    }

    double totalMegaBits = (static_cast<double>(totalPacketsReceived) * payloadSize * 8.0) / 1000000.0;
    double avgPps = ppsCollection.empty() ? 0.0 : static_cast<double>(totalPacketsReceived) / ppsCollection.size();
    double avgMbps = ppsCollection.empty() ? 0.0 : (avgPps * payloadSize * 8.0) / 1000000.0;
    double peakMbps = (static_cast<double>(peak) * payloadSize * 8.0) / 1000000.0;
    
    std::cout << "---- ---- Benchmark Results ---- ----" << std::endl;
    std::cout << "Total: " << totalPacketsReceived << " packets (" << totalMegaBits << " Mb)" << std::endl;
    std::cout << "Avg: " << avgPps << " pps (" << avgMbps << " Mbps)" << std::endl;
    std::cout << "Peak: " << peak << " pps (" << peakMbps << " Mbps)" << std::endl;
    
    std::cout << "Cleanup" << std::endl;
    while (this->clientSessionController->hasResponse()) {
        ttp2::Packet::Packet responsePacket = this->clientSessionController->popResponse();
    }
    std::cout << "done" << std::endl;
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
        {
          std::lock_guard<std::mutex> lock(this->takeBenchmarkValuesMutex);
          this->takeBenchmarkValues = true;
        }
        deltaStart = now;
      }
    }
  }

  void Benchmark::sendSession(const int packetSize) {
    std::string payload(std::max(0, packetSize), '0');
    ttp2::Packet::Packet packet;
    ttp2::Packet::Standard standard;
    standard.payload = payload;
    packet.payload = standard;

    while (isConnected()) {
        {
            std::unique_lock<std::mutex> lock(packetMutex);
            packetReceivedSignal.wait(lock, [&] { 
                return !isConnected() || inFlight < maxInFlight; 
            });
        }

        if (!isConnected()) break;

        this->clientSessionController->pushRequest(packet);

        {
            std::lock_guard<std::mutex> lock(packetMutex);
            inFlight++;
        }
    }
  }

  bool Benchmark::isConnected() {
    std::lock_guard<std::mutex> lock(connectedMutex);
    return connected;
  }

  void Benchmark::disconnect() {
    std::lock_guard<std::mutex> lock(connectedMutex);
    connected = false;
  }
}
