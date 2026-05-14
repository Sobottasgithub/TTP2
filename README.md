<h1 id="include">Include</h1>

# Use
To use TTP2 in your project you need to [include](#include) it in first. <br>
After including TTP2 to your project you can start to program the server.
1. Create a server tcp socket is set non blocking (SOCK_NONBLOCK):
   ```cpp
   sockaddr_in serverAddress;
   serverAddress.sin_family = AF_INET;
   serverAddress.sin_port = htons(PORT);
   serverAddress.sin_addr.s_addr = inet_addr(IP);
   
   int serverSocket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
   if(bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
      std::wcout << "Bind failed!" << std::endl;
      return -1;
   }
   ```
2. TTP2 operates internally using epoll; therefore, client accepts should also be handled with epoll.
3. Now you can create an instance of the ServerSessionController
   ```cpp
   auto serverSessionController = std::make_shared<ServerSessionController>(serverSocket, clientSocket);
   ```
4. Create a networkingSession thread
   ```cpp
   std::thread networkingSession([serverSessionController]() {
     serverSessionController->networkingSession();
   });
   ```
5. Take a look at the full implementation <a href="https://github.com/Sobottasgithub/TTP2/blob/develop/test/server/main.cpp">here</a>

Done! It is almost the same with the client:
1. Create a client tcp socket like such:
   ```cpp
   int serverSocket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);
    serverAddress.sin_addr.s_addr = inet_addr(IP);

    int connectionResult = connect(serverSocket, (struct sockaddr*) &serverAddress, sizeof(serverAddress));
    
    if (connectionResult < 0 && errno != EINPROGRESS) {
        std::wcout << "Connection failed!" << std::endl;
        return -1;
    }
   ```
2. Create an instance of the ClientSessionController
   ```cpp
   auto clientSessionController = std::make_shared<ClientSessionController>(serverSocket);
   ```
3. Create a networkingSession thread
   ```cpp
   std::thread networkThread([clientSessionController]() {
     clientSessionController->networkingSession();
   });
   ```
4. Take a look at the full implementation <a href="https://github.com/Sobottasgithub/TTP2/blob/develop/test/client/main.cpp">here</a>


Now you can enjoy TTP2 with the functions provided below:
1. Common:
   - void networkingSession();
   - bool isConnected()
   - void disconnect()
   - bool hasRequest()
   - bool hasResponse()
   - Packet popRequest()
   - Packet popResponse()
   - int getRequestQueueSize()
   - int getResponseQueueSize()
   - void pushResponse(Packet)
   - void pushRequest(Packet request)
  
   - int sendMessage(int socket, int id, int method, std::string payload)
   - int sendPacket(int socket, Packet packet)
   - Packet receiveMessage(int socket)

   - Packet
     ```cpp
     struct Packet {
        int id;
        int method;
        std::string payload;  
     };
     ```
   
2. ClientSessionController specific:
   - ClientSessionController();
   - ClientSessionController(int &socket);

3. ServerSessionController specific:
   - ServerSessionController();
   - ServerSessionController(int serverSocket, int clientSocket);
   - std::string getLocalIpAddress(std::string interface);

# Test
To test the TTP2 protocol you need to start the server first using:
```
nix run .#server
```
After that you can start the client with:
```
nix run .#client
```
