#include "client_session_controller.h"

#include <iostream>
#include <string>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <thread>
#include <regex>

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

int main() {
    std::string ipAddress = requestString("Server ipv4 (string): ");
    int port = requestInt("Server port (int): ");

    int serverSocket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = inet_addr(ipAddress.c_str());

    int connectionResult = connect(serverSocket, (struct sockaddr*) &serverAddress, sizeof(serverAddress));
    
    if (connectionResult < 0 && errno != EINPROGRESS) {
        std::wcout << "Connection failed!" << std::endl;
        return -1;
    }
    
    auto clientSessionController = std::make_shared<ClientSessionController>(serverSocket);

    std::thread networkThread([clientSessionController]() {
        clientSessionController->networkingSession();
    });

    int count = 0;
    while (true) {
        if (!clientSessionController->isConnected()) {
            std::wcout << "Disconnect!" << std::endl;
            break;
        }

        int option = requestInt("Choose option\n(1) Send message\n(2) Read messages\n(3) Benchmark\n(4) Exit\nnumber: ");
        if (option == 1) {
            std::string payload = requestString("(string) Payload: ");
        
            ClientSessionController::Packet packet;
            packet.id = count;
            Networking::Standard standard;
            standard.payload = payload;
            packet.payload = standard;
            count++;
            clientSessionController->pushRequest(packet);
        } else if (option == 2) {
            if (!clientSessionController->hasResponse()) {
                std::wcout << "No Messages!" << std::endl;
                continue;
            }
            while(clientSessionController->hasResponse()) {
                ClientSessionController::Packet packet = clientSessionController->popResponse();
                Networking::Standard standard = std::get<Networking::Standard>(packet.payload);
                std::wcout << "------ Message ------" << std::endl;
                std::wcout << "ID: " << packet.id << std::endl;
                std::wcout << "Payload: " << standard.payload.c_str() << std::endl;
                std::wcout << "---------------------" << std::endl;
            }
        } else if (option == 3) {
            std::wcout << "~~~~~~ ~~~~~~ Benchmark ~~~~~~ ~~~~~~" << std::endl;
            option = requestInt("Choose option\n(1) Send continious stream\n(2) Send n packages\nnumber: ");
            if (option == 1) {
                std::string payload = requestString("(string) Payload: ");
        
                ClientSessionController::Packet packet;

                while (true) {
                    packet.id = count;
                    Networking::Standard standard;
                    standard.payload = payload;
                    packet.payload = standard;
                    count++;
                    clientSessionController->pushRequest(packet);
                    while(clientSessionController->hasResponse()) {
                        ClientSessionController::Packet packet = clientSessionController->popResponse();
                        Networking::Standard standard = std::get<Networking::Standard>(packet.payload);
                        std::wcout << "------ Message ------" << std::endl;
                        std::wcout << "ID: " << packet.id << std::endl;
                        std::wcout << "Payload: " << standard.payload.c_str() << std::endl;
                        std::wcout << "---------------------" << std::endl;
                    }
                }
            } else if (option == 2) {
                int count = requestInt("(int) Packet count: ");
                std::string payload = requestString("(string) Payload: ");
        
                ClientSessionController::Packet packet;
                packet.id = count;

                Networking::Standard standard;
                standard.payload = payload;
                packet.payload = standard;
                count++;

                for (int index = 0; index < count; index++) {
                    clientSessionController->pushRequest(packet);
                }
                while (count != 0) {
                    if (clientSessionController->hasResponse()) {
                        ClientSessionController::Packet packet = clientSessionController->popResponse();
                        Networking::Standard standard = std::get<Networking::Standard>(packet.payload);
                        std::wcout << "------ Message ------" << std::endl;
                        std::wcout << "ID: " << packet.id << std::endl;
                        std::wcout << "Payload: " << standard.payload.c_str() << std::endl;
                        std::wcout << "---------------------" << std::endl;
                        count--;
                    }
                }                
            } else {
                std::wcout << "Invalid!" << std::endl;
            }
        } else if (option == 4) {
          clientSessionController->disconnect();  
        } else {
            std::wcout << "Invalid!" << std::endl;
        }
    }
    networkThread.detach();

    return 0;
}
