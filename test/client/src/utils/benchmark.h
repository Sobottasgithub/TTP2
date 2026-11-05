#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <memory>
#include <client_session_controller.h>

namespace test::benchmark {
  class Benchmark {
    public:
      Benchmark(std::shared_ptr<ttp2::ClientSessionController> clientSessionController);
    private:
      std::shared_ptr<ttp2::ClientSessionController> clientSessionController;
  };
}

#endif
