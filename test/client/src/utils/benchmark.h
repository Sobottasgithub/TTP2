#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <memory>
#include <client_session_controller.h>
#include <mutex>
#include <condition_variable>

namespace test::benchmark {
  class Benchmark {
    public:
      Benchmark(std::shared_ptr<ttp2::ClientSessionController> clientSessionController);
    private:
      bool connected = true;
      std::mutex connectedMutex;
      std::mutex packetMutex;
      std::condition_variable packetReceivedSignal;
      long long inFlight = 0;
      const long long maxInFlight = 500;
      std::shared_ptr<ttp2::ClientSessionController> clientSessionController;
      std::mutex takeBenchmarkValuesMutex;
      bool takeBenchmarkValues = false;

      void timerSession(const int duration);
      void sendSession(const int packetSize);
      bool isConnected();
      void disconnect();
  };
}

#endif
