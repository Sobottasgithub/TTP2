#include "client_session_controller.h"

#include <iostream>
#include <string>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <thread>
#include <memory>

int main() {
    std::string ipAddress;
    int port;

    std::wcout << "Server ipv4 (string): ";
    std::cin >> ipAddress;
    std::wcout << "Server port (int): ";
    std::cin >> port; 

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = inet_addr(ipAddress.c_str());

    if(connect(serverSocket, (struct sockaddr*) &serverAddress, sizeof(serverAddress)) == 0) {
        auto clientSessionController = std::make_shared<ClientSessionController>(serverSocket);

        std::thread networkThread([clientSessionController]() {
            clientSessionController->networkingSession();
        });

        while (true) {
            if (!clientSessionController->isConnected()) {
                std::wcout << "Disconnect!" << std::endl;
                break;
            }
    
            std::wcout << "Choose option\n(1) send message\n(2) read messages\n(3) exit\nnumber: ";
            int option;
            std::cin >> option;

            if (option == 1) {
                ClientSessionController::Packet packet;
                int method;
                std::string payload;

                std::wcout << "(int) Method: ";
                std::cin >> method; // TODO: Check type
                std::wcout << "(string) Payload: ";
                std::cin >> payload;
        
                packet.method  = method;
                packet.payload = payload;
                clientSessionController->pushRequest(packet);
            } else if (option == 2) {
                if (!clientSessionController->hasResponse()) {
                    std::wcout << "No Messages!" << std::endl;
                    continue;
                }
                while(clientSessionController->hasResponse()) {
                    ClientSessionController::Packet packet = clientSessionController->popResponse();
                    std::wcout << "------ Message ------" << std::endl;
                    std::wcout << "Method:  " << packet.method << std::endl;
                    std::wcout << "Payload: " << packet.payload.c_str() << std::endl;
                    std::wcout << "---------------------" << std::endl;
                }
            } else if (option == 3) {
              clientSessionController->disconnect();  
            } else {
                std::wcout << "Invalid!" << std::endl;
            }
        }
        networkThread.detach();
    }

    return 0;
}
