#ifndef CLIENT_SESSION_CONTROLLER_H
#define CLIENT_SESSION_CONTROLLER_H

#include "../src/networking.h"

class ClientSessionController: public Networking
{
    public:
      void networkingSession(int socket);
      void testAsn1();
      static int write_callback(const void *buffer, size_t size, void *app_key); 
};

#endif
