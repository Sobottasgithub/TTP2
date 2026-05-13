#include "server_session_controller.h"

#include <iostream>
#include <string>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <thread>
#include <memory>
#include <chrono>
#include <regex>
#include <sys/epoll.h>

bool isNumeric(const std::string& string) {
  static const std::regex numberRegex(
      R"(^[-+]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][-+]?\d+)?$)"
  );
  return std::regex_match(string, numberRegex);
}

std::string requestString(const std::string& message) {
    std::string userInput;
    std::wcout << message.c_str();
    std::getline(std::cin, userInput);
    return userInput;
}

int requestInt(const std::string& message) {
    std::string userInput;

    while (true) {
        std::wcout << message.c_str();
        std::getline(std::cin, userInput);

        if (isNumeric(userInput)) {
            return std::stoi(userInput);
        }
        std::wcout << "Invalid input! Try again\n";
    }
}

void clientManager(int serverSocket, int clientSocket) {
    auto serverSessionController = std::make_shared<ServerSessionController>(serverSocket, clientSocket);

    std::thread networkingSession([serverSessionController]() {
        serverSessionController->networkingSession();
    });

    int messageCounter = 0;
    while (serverSessionController->isConnected()) {
        if (serverSessionController->hasRequest()) {
            messageCounter++;
            // std::wcout << "Received request" << messageCounter << "!" << std::endl;
            ServerSessionController::Packet packet = serverSessionController->popRequest();
            std::string payload = packet.payload;
            std::wcout << "payload: " << payload.c_str() << std::endl;
            serverSessionController->pushResponse(packet);
        }
    }

    std::wcout << "Terminated!" << std::endl;
    networkingSession.detach();    
}

int main() {
    std::string interface = requestString("Interface (string): ");
    int port = requestInt("Server port (int): ");

    ServerSessionController tempServerSessionController;
    std::string containerIP = tempServerSessionController.getLocalIpAddress(interface);

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = inet_addr(containerIP.c_str());

    int serverSocket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress));

    // Create epoll
    int epollFd = epoll_create1(0);
    if (epollFd == -1) {
        std::wcout << "Failed to create epoll!" << std::endl;
    }
    // Set epoll action for server
    struct epoll_event serverEvents;
    serverEvents.events = EPOLLIN | EPOLLOUT;
    serverEvents.data.fd = serverSocket;
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, serverSocket, &serverEvents) == -1) {
        std::wcout << "Failed to set epoll_ctl!" << std::endl;
        return 1;
    }

    listen(serverSocket, 5);
    std::vector<std::thread> clientConnections;
    while (true) {
        const int MAX_EVENTS = 10;
        struct epoll_event events[MAX_EVENTS];
        int epollRequestCount = epoll_wait(epollFd, events, MAX_EVENTS, -1);
        
        for (int index = 0; index < epollRequestCount; ++index) {
            if (events[index].data.fd == serverSocket) {
                int clientSocket = accept4(serverSocket, nullptr, nullptr, SOCK_NONBLOCK);
                std::wcout << "New clientSocket: " << clientSocket << std::endl;

                clientConnections.push_back(std::thread(
                                               clientManager,
                                               serverSocket,
                                               clientSocket
                                            ));
            }
        }
    }

    return 0;
}
