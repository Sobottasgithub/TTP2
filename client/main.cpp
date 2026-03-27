#include "src/networking.h"

#include <iostream>
#include <string>

int main() {
    std::string ipAdress;
    int port;
    
    std::wcout << "Server ipv4: ";
    std::cin >> ipAdress;
    std::wcout << "Server port: ";
    std::cin >> port; 
    
    Networking();
    
    return 0;
}
