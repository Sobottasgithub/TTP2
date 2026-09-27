#ifndef CLI_H
#define CLI_H

#include "client_session_controller.h"
#include <memory>

namespace test {
  class Cli {
    public:
      Cli();
    private:
      std::shared_ptr<ttp2::ClientSessionController> clientSessionController;
      
      void readMessages();
      void sendMessagePacket();
      void benchmark();
      void filePacket();
      void peekIndex();
      void viewportPacket();
      void tqlQueryPacket();
      void errorPacket();
      void tdfsOptions();
  };
}

#endif
