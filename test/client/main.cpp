#include "client_session_controller.h"

#include <arrow/csv/options.h>
#include <arrow/table.h>
#include <iostream>
#include <memory>
#include <string>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <thread>
#include <regex>
#include <filesystem>
#include <arrow/csv/api.h>
#include <arrow/io/api.h>

using namespace ttp2;

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

std::shared_ptr<arrow::Table> openCsvFile() {
    std::string filePath { "" };
    do {
      if (filePath.length() > 0 && !std::filesystem::exists(filePath)) {
          std::wcout << "Incorrect filepath!" << std::endl;
      }
      filePath = requestString("(string) Filepath: ");
    } while (!std::filesystem::exists(filePath));

    arrow::io::IOContext ioContext = arrow::io::default_io_context();

    arrow::Result<std::shared_ptr<arrow::io::ReadableFile>> maybeFile = arrow::io::ReadableFile::Open(filePath);
    std::shared_ptr<arrow::io::InputStream> fileInput = *maybeFile;

    arrow::csv::ReadOptions readOptions = arrow::csv::ReadOptions::Defaults();
    arrow::csv::ParseOptions parseOptions = arrow::csv::ParseOptions::Defaults();
    arrow::csv::ConvertOptions convertOptions = arrow::csv::ConvertOptions::Defaults();

    arrow::Result<std::shared_ptr<arrow::csv::TableReader>> maybeReader = arrow::csv::TableReader::Make(ioContext,
                                                    fileInput,
                                                    readOptions,
                                                    parseOptions,
                                                    convertOptions);
    if (!maybeReader.ok()) {
     std::wcout << "Error while instantiating TableReader!" << std::endl;
    }
    std::shared_ptr<arrow::csv::TableReader> reader = *maybeReader;

    arrow::Result<std::shared_ptr<arrow::Table>> maybeTable = reader->Read();
    if (!maybeTable.ok()) {
      std::wcout << "Error while read table from CSV file!" << std::endl;
    }
    std::shared_ptr<arrow::Table> table = *maybeTable;
    return table;
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

    while (clientSessionController->isConnected()) {
        int option = requestInt("Choose option\n(1) Send message\n(2) Read messages\n(3) Benchmark\n(4) Open file\n(5) peek index\n(6) Viewport\n(7) Exit\nnumber: ");
        if (option == 1) {
            std::string payload = requestString("(string) Payload: ");
        
            ClientSessionController::Packet packet;
            Networking::Standard standard;
            standard.payload = payload;
            packet.payload = standard;
            clientSessionController->pushRequest(packet);
        } else if (option == 2) {
            if (!clientSessionController->hasResponse()) {
                std::wcout << "No Messages!" << std::endl;
                continue;
            }
            while(clientSessionController->hasResponse()) {
                ClientSessionController::Packet packet = clientSessionController->popResponse();

                if (std::holds_alternative<Networking::Standard>(packet.payload)) {
                    Networking::Standard standard = std::get<Networking::Standard>(packet.payload);
                    std::wcout << "------ Message ------" << std::endl;
                    std::wcout << "ID: " << packet.id << std::endl;
                    std::wcout << "Payload: " << standard.payload.c_str() << std::endl;
                    std::wcout << "---------------------" << std::endl;
                } else if (std::holds_alternative<Networking::File>(packet.payload)) {
                    Networking::File file = std::get<Networking::File>(packet.payload);
                    std::wcout << "------ Message ------" << std::endl;
                    std::wcout << "ID: " << packet.id << std::endl;
                    std::wcout << file.payload->ToString().c_str() << std::endl;
                    std::wcout << "---------------------" << std::endl;
                } else if (std::holds_alternative<Networking::Viewport>(packet.payload)) {
                    Networking::Viewport viewport = std::get<Networking::Viewport>(packet.payload);
                    std::wcout << "------ Message Viewport------" << std::endl;
                    std::wcout << "ID: " << packet.id << std::endl;
                    std::wcout << viewport.payload->ToString().c_str() << std::endl;
                    std::wcout << "---------------------" << std::endl;
                }

            }
        } else if (option == 3) {
            std::wcout << "~~~~~~ ~~~~~~ Benchmark ~~~~~~ ~~~~~~" << std::endl;
            option = requestInt("Choose option\n(1) Send continious stream\n(2) Send n packages\nnumber: ");
            if (option == 1) {
                std::string payload = requestString("(string) Payload: ");
        
                ClientSessionController::Packet packet;
                while (true) {
                    Networking::Standard standard;
                    standard.payload = payload;
                    packet.payload = standard;
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
                Networking::Standard standard;
                standard.payload = payload;
                packet.payload = standard;

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
          std::shared_ptr<arrow::Table> table = openCsvFile();
          ClientSessionController::Packet packet;
          ClientSessionController::File file;
          file.start = 0;
          file.end = table->num_rows();
          file.payload = table;
          packet.payload = file;
          clientSessionController->pushRequest(packet);

          std::wcout << "Done!" << std::endl;
        } else if (option == 5) {
          int responseQueueSize = clientSessionController->getResponseQueueSize();
          int index = 0;
          do {
              std::wcout << "Peek index: 0 to " << responseQueueSize << " | -1 to exit" << std::endl;
              index = requestInt("(int) index: ");
          } while (index < -1 || index > responseQueueSize);

          if (index == -1) {
              continue;
          }

          ClientSessionController::PacketInfo packetInfo = clientSessionController->peekResponse(index);
          std::wcout << "--- PacketInfo ---" << std::endl;
          std::string payloadType = "";
          if (std::holds_alternative<ClientSessionController::Standard>(packetInfo.payloadType))
              payloadType = "Standard";
          else if (std::holds_alternative<ClientSessionController::File>(packetInfo.payloadType))
              payloadType = "File";
          else if (std::holds_alternative<ClientSessionController::Viewport>(packetInfo.payloadType))
              payloadType = "Viewport";
          else
              payloadType = "Invalid";
          
          std::wcout << "id: " << packetInfo.id << "\npacketType: " << payloadType.c_str() << std::endl;
          std::wcout << "---    ---     ---" << std::endl;
        } else if (option == 6) {
          std::shared_ptr<arrow::Table> table = openCsvFile();
          ClientSessionController::Packet packet;
          ClientSessionController::Viewport viewport;
          viewport.xStart = 0;
          viewport.xEnd = table->num_rows();
          viewport.yStart = 0;
          viewport.yEnd = table->num_columns();
          viewport.payload = table;
          packet.payload = viewport;
          clientSessionController->pushRequest(packet);

          std::wcout << "Done!" << std::endl;
        } else if (option == 7) {
          clientSessionController->disconnect();  
        } else {
            std::wcout << "Invalid!" << std::endl;
        }
    }

    std::wcout << "Terminated!" << std::endl;
    
    networkThread.join();

    return 0;
}
