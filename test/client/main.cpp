#include "src/networking.h"

#include <iostream>
#include <string>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <thread>

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

    if(connect(serverSocket, (struct sockaddr*) &serverAddress, sizeof(serverAddress)) == -1) {
        helpers::Packet responseCode = helpers::receiveMessage(serverSocket);

        Networking networking;

        std::thread networkThread(
            &Networking::networkingSession,
            &networking,
            serverSocket
        );

        while (true) {
            if (!networking.isConnected()) {
                std::wcout << "Disconnect!" << std::endl;
                break;
            }
            
            std::wcout << "Choose option\n(1) send message\n(2) read messages\nnumber: ";
            int option;
            std::cin >> option;

            if (option == 1) {
                helpers::Packet packet;
                int method;
                std::string payload;

                std::wcout << "(int) Method: ";
                std::cin >> method; // TODO: Check type
                std::wcout << "(string) Payload";
                std::cin >> payload;
                
                packet.method  = method;
                packet.payload = payload;
                networking.pushOrder(packet);
            } else if (option == 2) {
                if (!networking.hasSolution()) {
                    std::wcout << "No Messages!" << std::endl;
                    continue;
                }
                while(networking.hasSolution()) {
                    helpers::Packet packet = networking.popSolution();
                    std::wcout << "-- Message --" << std::endl;
                    std::wcout << "Method:  " << packet.method << std::endl;
                    std::wcout << "Payload: " << packet.payload.c_str() << std::endl;
                    std::wcout << "-------------" << std::endl;
                }
            } else {
                std::wcout << "Invalid!" << std::endl;
            }
        }
    }
    
    return 0;
}
