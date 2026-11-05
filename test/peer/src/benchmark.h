#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <peer_session_controller.h>
#include <memory.h>

namespace test::benchmark {
  class Benchmark {
    public:
      Benchmark(std::shared_ptr<ttp2::PeerSessionController> peerSessionController);
    private:
      
  };
}

#endif
