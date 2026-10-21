# Include
Add the following to your flake:
```nix
inputs = {
    ...
    ttp2 = {
      url = "github:Sobottasgithub/ttp2";
    };
    ...
};
```
After that you can just use it like a normal lib.
Now add the following to your Cmake:
```CMake
target_link_libraries(YOURNAME PRIVATE ttp2)
```
# Types
The following types are supported:
```cpp
  struct Standard {
    std::string payload = "";
  };

  struct File {
  	std::string filePath = "";
    int start = -1;
  	int end = -1;
  	std::shared_ptr<arrow::Table> payload = arrow::Table::Make(arrow::schema({}), std::vector<std::shared_ptr<arrow::Array>>{}, 0);
  };

  struct ViewportRequest {
    int xStart = 0;
    int xEnd = 0;
    int yStart = 0;
    int yEnd = 0;
  };

  struct Viewport {
    int xStart = 0;
    int xEnd = 0;
    int yStart = 0;
    int yEnd = 0;
    std::shared_ptr<arrow::Table> payload = arrow::Table::Make(arrow::schema({}), std::vector<std::shared_ptr<arrow::Array>>{}, 0);
  };

  struct TqlQuery {
    std::string query = "";
  };

  struct Error {
    int code = 0;
    std::string message = "";
  };

  struct Universal {
    int type;
    std::vector<uint8_t> bytes;
  };
```
<b>TDFS packet types</b>
```cpp
    struct Ls {
      std::string directory;
    };

    struct LsSolution {
      std::vector<std::string> content;
    };

```
Then use it in the Packet wrapper to send it later:
```cpp
struct Packet {
    int id = -1;
    payloadVariants payload;  
};
```
# Use
## UDP
To use TTP2 in your project you need to [include](#include) it in first. <br>
After including TTP2 to your project you can start to program the server.
```cpp
    #include <peer_session_controller.h>
```
After including the peer_session_controller you can initialize it via:
```cpp
    auto peerSessionController = std::make_shared<ttp2::PeerSessionController>(ip, port);
    std::thread networkingSession([peerSessionController]() {
        peerSessionController->networkingSession();
    });
```
Then you can use the following api functions:
- void sendPacket(AddressedPacket addressedPacket);
- AddressedPacket popReceivedPacket();
- bool hasPacket();

- bool isConnected();
- void disconnect();

## TCP
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
## TCP
To test the TTP2 TCP protocol you need to start the server first using:
```
nix run .#server
```
After that you can start the client with:
```
nix run .#client
```

## UDP
To test the TTP2 UDP protocol you need to start at least two peers on one or more devices:
```
nix run .#peer
```
If you run more that one peers on one device make sure to use different ports.
