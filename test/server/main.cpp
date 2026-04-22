#include "server_session_controller.h"

#include <iostream>
#include <string>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <thread>
#include <memory>
#include <chrono>

void clientManager(int serverSocket, int clientSocket) {
    std::wcout << "clientSocket: " << clientSocket << std::endl;

    auto serverSessionController = std::make_shared<ServerSessionController>(serverSocket, clientSocket);

    std::thread networkingSession([serverSessionController]() {
        serverSessionController->networkingSession();
    });
    
    while (serverSessionController->isConnected()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        if (serverSessionController->hasRequest()) {
            std::wcout << "Received request!" << std::endl;
            ServerSessionController::Packet packet = serverSessionController->popRequest();
            serverSessionController->pushResponse(packet);
        }
    }

    std::wcout << "Terminated!" << std::endl;
    networkingSession.detach();    
}

int main() {
    std::string interface;
    int port;

    std::wcout << "Interface (string): ";
    std::cin >> interface;
    std::wcout << "Port (int): ";
    std::cin >> port;

    ServerSessionController tempServerSessionController;
    std::string containerIP = tempServerSessionController.getLocalIpAddress(interface);

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = inet_addr(containerIP.c_str());

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress));

    listen(serverSocket, 5);
    std::vector<std::thread> clientConnections;
    while (true) {
        int clientSocket = accept(serverSocket, nullptr, nullptr);
        std::wcout << "New clientSocket: " << clientSocket << std::endl;
        clientConnections.push_back(std::thread(
                                       clientManager,
                                       serverSocket,
                                       clientSocket
                                    ));
    }

    return 0;
}
