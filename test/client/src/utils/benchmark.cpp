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
    std::cout << "~~~~~~ ~~~~~~ 1 Minute Throughput Benchmark ~~~~~~ ~~~~~~" << std::endl;
    int payloadSize = test::Helpers::requestInt("(Int) payload size (1024): ");

    std::thread sendSessionThread([this, payloadSize] {
        this->sendSession(payloadSize);
    });

    auto start = std::chrono::steady_clock::now();
    auto deltaStart = start;

    std::vector<int> ppsCollection;
    ppsCollection.reserve(65);

    long long totalBytesSent = 0;
    int maxPacketsPerSecond = 0;
    int receivedPacketsThisSecond = 0;

    const int benchmarkDurationSeconds = 60;

    std::cout << "Running benchmark for 60 seconds..." << std::endl;
    while (true) {
        auto now = std::chrono::steady_clock::now();
        auto elapsedTotal = std::chrono::duration_cast<std::chrono::seconds>(now - start).count();
        if (elapsedTotal >= benchmarkDurationSeconds) {
            break;
        }

        bool processedAny = false;
        while (this->clientSessionController->hasResponse()) {
            auto responsePacket = this->clientSessionController->popResponse();
            receivedPacketsThisSecond++;
            totalBytesSent += payloadSize;
            processedAny = true;

            {
                std::lock_guard<std::mutex> lock(packetMutex);
                inFlight--;
            }
            packetReceivedSignal.notify_one();
        }

        auto deltaElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - deltaStart).count();
        if (deltaElapsed >= 1000) {
            deltaStart = now;
            if (receivedPacketsThisSecond > maxPacketsPerSecond) {
                maxPacketsPerSecond = receivedPacketsThisSecond;
            }
            ppsCollection.push_back(receivedPacketsThisSecond);
            receivedPacketsThisSecond = 0;
        }

        if (!processedAny) {
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    }

    // INFO: This only disconnects the Benchmark not the clientSessionController
    disconnect();
    packetReceivedSignal.notify_all();

    auto end = std::chrono::steady_clock::now();
    auto totalSeconds = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();

    if (sendSessionThread.joinable()) {
        sendSessionThread.join();
    }
    std::cout << "Done joining threads" << std::endl;

    int avgPps = 0;
    if (!ppsCollection.empty()) {
        int sum = 0;
        for (size_t index = 0; index < ppsCollection.size(); index++) {
            sum += ppsCollection.at(index);
        }
        avgPps = sum / ppsCollection.size();
    } else {
        avgPps = receivedPacketsThisSecond;
    }

    double totalMegabits = (totalBytesSent * 8.0) / 1000000.0;
    double throughputMbps = (totalSeconds > 0) ? (totalMegabits / totalSeconds) : 0.0;

    std::cout << "\n--- Benchmark Results ---" << std::endl;
    std::cout << "Duration: " << totalSeconds << " seconds" << std::endl;
    std::cout << "Total Data Sent: " << (totalBytesSent / 1024 / 1024) << " MB (" << totalMegabits << " Mb)" << std::endl;
    std::cout << "Throughput: " << throughputMbps << " Mbps" << std::endl;
    std::cout << maxPacketsPerSecond << " pps max; " << avgPps << " pps AVG" << std::endl;
    
    std::cout << "Cleanup" << std::endl;
    while (this->clientSessionController->hasResponse()) {
        ttp2::Packet::Packet responsePacket = this->clientSessionController->popResponse();
    }
    std::cout << "done" << std::endl;
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
