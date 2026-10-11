#ifndef SEND_SESSION_CONTROLLER
#define SEND_SESSION_CONTROLLER

#include "networking.h"

namespace ttp2 {
  class SendSessionController: public Networking {
      public:
        SendSessionController(int clientSocket);
        void sendResponseSession();
        void disconnect() override;

        bool hasRequest() = delete;
        Packet::Packet popRequest() = delete;
        int getRequestQueueSize() = delete;
        void pushRequest(Packet::Packet request) = delete;
        PacketInfo peekRequest() = delete;
        PacketInfo peekRequest(int index) = delete;

      private:
        int clientSocket;
  };
}

#endif
