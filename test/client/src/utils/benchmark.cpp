#include "benchmark.h"
#include "helpers.h"

#include <iostream>

#include <chrono>
#include <packet_types.h>
#include <vector>

namespace test::benchmark {
  Benchmark::Benchmark(std::shared_ptr<ttp2::ClientSessionController> clientSessionController) {
    this->clientSessionController = clientSessionController;
    std::cout << "~~~~~~ ~~~~~~ 1 Minute Throughput Benchmark ~~~~~~ ~~~~~~" << std::endl;
    int payloadSize = test::Helpers::requestInt("(Int) payload size (1460): ");

    std::string payload(std::max(0, payloadSize), '0');

    ttp2::Packet::Packet packet;
    ttp2::Packet::Standard standard;
    standard.payload = payload;
    packet.payload = standard;

    auto start = std::chrono::steady_clock::now();
    auto deltaStart = start;

    std::vector<int> ppsCollection;
    ppsCollection.reserve(65);

    long long totalBytesSent = 0;
    int maxPacketsPerSecond = 0;
    int receivedPacketsThisSecond = 0;

    const int benchmarkDurationSeconds = 60;
    const uint64_t timeCheckInterval = 4096;
    uint64_t iterations = 0;

    std::cout << "Running benchmark for 60 seconds..." << std::endl;
    long long inFlight = 0;
    const long long maxInFlight = 500;
    while (true) {
        if (inFlight < maxInFlight) {
            this->clientSessionController->pushRequest(packet);
            totalBytesSent += payloadSize;
            inFlight++;
        }

        while (this->clientSessionController->hasResponse()) {
            auto responsePacket = this->clientSessionController->popResponse();
            receivedPacketsThisSecond++;
            inFlight--;
        }
    
        iterations++;
        if ((iterations & (timeCheckInterval - 1)) == 0) {
            auto now = std::chrono::steady_clock::now();

            auto elapsedTotal = std::chrono::duration_cast<std::chrono::seconds>(now - start).count();
            if (elapsedTotal >= benchmarkDurationSeconds) {
                break;
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
        }
    }

    auto end = std::chrono::steady_clock::now();
    auto totalSeconds = std::chrono::duration_cast<std::chrono::seconds>(end - start).count();

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
}
