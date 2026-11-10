#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <peer_session_controller.h>
#include <memory.h>

namespace test::benchmark {
  class Benchmark {
    public:
      Benchmark(std::shared_ptr<ttp2::PeerSessionController> peerSessionController);
    private:
      std::shared_ptr<ttp2::PeerSessionController> peerSessionController;
      std::mutex connectedMutex;
      bool connected = true;

      void sendSession();
      void sendBenchmarkPackets(const char* ip, const int port, const int payloadSize);
      bool isConnected();
      void disconnect();
      
      void returnSession();
  };
}

#endif
