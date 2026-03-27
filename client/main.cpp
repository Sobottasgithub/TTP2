#include "src/networking.h"

#include <iostream>
#include <string>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

int main() {
    std::string ipAddress;
    int port;
    
    std::wcout << "Server ipv4: ";
    std::cin >> ipAddress;
    std::wcout << "Server port: ";
    std::cin >> port; 


    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = inet_addr(ipAddress.c_str());

    if(connect(serverSocket, (struct sockaddr*) &serverAddress, sizeof(serverAddress)) == -1) {
        helpers::Packet responseCode = helpers::receiveMessage(serverSocket);
        Networking(serverSocket);
    }
    
    return 0;
}
