#include "server_session_controller.h"

#include <iostream>
#include <string>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <thread>
#include <chrono>

int main() {
    std::string interface;
    int port;

    std::wcout << "Interface (string): ";
    std::cin >> interface;
    std::wcout << "Port (int): ";
    std::cin >> port;

    ServerSessionController tempServerSessionController(1, 1);
    std::string containerIP = tempServerSessionController.getLocalIpAddress(interface);

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = inet_addr(containerIP.c_str());

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress));

    listen(serverSocket, 5);
    int clientSocket = accept(serverSocket, nullptr, nullptr);
    std::wcout << "clientSocket: " << clientSocket << std::endl;
    
    ServerSessionController serverSessionController(serverSocket, clientSocket);
    std::thread networkingSession = std::thread(&ServerSessionController::networkingSession,
                                                &serverSessionController
                                               );

    while (true) {
        std::wcout << "Checking for messages..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        if (serverSessionController.hasOrder()) {
            std::wcout << "Has order!" << std::endl;
        }
    }

    std::wcout << "Terminated!" << std::endl;

    if (networkingSession.joinable()) {
      networkingSession.join();
    }
    
    return 0;
}
