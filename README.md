<h1 id="include">Include</h1>

# Use
To use TTP2 in your project you need to [include](#include) it in first. <br>
After including TTP2 to your project you can start to program the server.
1. Create a server tcp socket
2. Create an instance of the ServerSessionController
   ```cpp
   auto serverSessionController = std::make_shared<ServerSessionController>(serverSocket, clientSocket);
   ```
3. Create a networkingSession thread
   ```cpp
   std::thread networkingSession([serverSessionController]() {
     serverSessionController->networkingSession();
   });
   ```
Done! It is almost the same with the client:
1. Create a client tcp socket
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
  
   - int sendMessage(int socket, int method, std::string payload)
   - int sendPacket(int socket, Packet packet)
   - Packet receiveMessage(int socket)

   - Packet
     ```cpp
     struct Packet {
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

# How does it work?
In the background the networkingSession of the clientSessionController and the serverSessionController do the following:
<img width="700" height="800" alt="TTP2Protocol-Done" src="https://github.com/user-attachments/assets/67ec87ff-1300-4601-a7ec-3b17c006bd46" />
