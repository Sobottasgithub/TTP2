#include "cli.h"
#include "helpers.h"

#include "client_session_controller.h"
#include "packet_types.h"
#include "tdfs_packet_types.h"

#include <arrow/csv/options.h>
#include <arrow/table.h>
#include <iostream>
#include <memory>
#include <string>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <thread>
#include <chrono>
#include <arrow/csv/api.h>
#include <arrow/io/api.h>

namespace test {
  Cli::Cli() {
    std::string ipAddress = test::Helpers::requestString("Server ipv4 (string): ");
    int port = test::Helpers::requestInt("Server port (int): ");
    
    auto clientSessionController = std::make_shared<ttp2::ClientSessionController>(ipAddress, port);

    std::thread networkThread([clientSessionController]() {
        clientSessionController->networkingSession();
    });

    while (clientSessionController->isConnected()) {
        int option = test::Helpers::requestInt("Choose option\n(0) Exit\n(1) Send message\n(2) Read messages\n(3) Benchmark\n(4) Open file\n(5) peek index\n(6) Viewport\n(7) TqlQuery\n(8) Error\n(9) TDFS\nnumber: ");
        if (option == 0) {
          clientSessionController->disconnect();  
        } else if (option == 1) {
            std::string payload = test::Helpers::requestString("(string) Payload: ");
        
            ttp2::Packet::Packet packet;
            ttp2::Packet::Standard standard;
            standard.payload = payload;
            packet.payload = standard;
            clientSessionController->pushRequest(packet);
        } else if (option == 2) {
            if (!clientSessionController->hasResponse()) {
                std::cout << "No Messages!" << std::endl;
                continue;
            }
            while(clientSessionController->hasResponse()) {
                ttp2::Packet::Packet packet = clientSessionController->popResponse();

                if (std::holds_alternative<ttp2::Packet::Standard>(packet.payload)) {
                    ttp2::Packet::Standard standard = std::get<ttp2::Packet::Standard>(packet.payload);
                    std::cout << "------ Message ------" << std::endl;
                    std::cout << "ID: " << packet.id << std::endl;
                    std::cout << "Payload: " << standard.payload << std::endl;
                    std::cout << "---------------------" << std::endl;
                } else if (std::holds_alternative<ttp2::Packet::File>(packet.payload)) {
                    ttp2::Packet::File file = std::get<ttp2::Packet::File>(packet.payload);
                    std::cout << "------ Viewport  ------" << std::endl;
                    std::cout << "ID: " << packet.id << std::endl;
                    std::cout << file.payload->ToString() << std::endl;
                    std::cout << "---------------------" << std::endl;
                } else if (std::holds_alternative<ttp2::Packet::Viewport>(packet.payload)) {
                    ttp2::Packet::Viewport viewport = std::get<ttp2::Packet::Viewport>(packet.payload);
                    std::cout << "------ Message Viewport------" << std::endl;
                    std::cout << "ID: " << packet.id << std::endl;
                    std::cout << viewport.payload->ToString() << std::endl;
                    std::cout << "---------------------" << std::endl;
                } else if (std::holds_alternative<ttp2::Packet::TqlQuery>(packet.payload)) {
                    ttp2::Packet::TqlQuery tqlQuery = std::get<ttp2::Packet::TqlQuery>(packet.payload);
                    std::cout << "------ TQL Query ------" << std::endl;
                    std::cout << "ID: " << packet.id << std::endl;
                    std::cout << "Query: " << tqlQuery.query << std::endl;
                    std::cout << "---------------------" << std::endl;
                } else if (std::holds_alternative<ttp2::Packet::Error>(packet.payload)) {
                    ttp2::Packet::Error error = std::get<ttp2::Packet::Error>(packet.payload);
                    std::cout << "------ Error ------" << std::endl;
                    std::cout << "ID: " << packet.id << std::endl;
                    std::cout << "Code: " << error.code << std::endl;
                    std::cout << "Message: " << error.message << std::endl;
                    std::cout << "---------------------" << std::endl;
                } else if (std::holds_alternative<ttp2::Packet::tdfs::Ls>(packet.payload)) {
                    ttp2::Packet::tdfs::Ls ls = std::get<ttp2::Packet::tdfs::Ls>(packet.payload);
                    std::cout << "------ TDFS LS ------" << std::endl;
                    std::cout << "ID: " << packet.id << std::endl;
                    std::cout << "Directory: " << ls.directory << std::endl;
                    std::cout << "---------------------" << std::endl;
                } else if (std::holds_alternative<ttp2::Packet::tdfs::LsSolution>(packet.payload)) {
                    ttp2::Packet::tdfs::LsSolution lsSolution = std::get<ttp2::Packet::tdfs::LsSolution>(packet.payload);
                    std::cout << "------ TDFS LS ------" << std::endl;
                    std::cout << "ID: " << packet.id << std::endl;
                    std::cout << "CONTENT: " << std::flush;
                    for (const std::string& item : lsSolution.content) {
                        std::cout << item << std::flush;
                    }
                    std::cout << std::endl;
                    std::cout << "---------------------" << std::endl;
                }
            }
        } else if (option == 3) {
            std::cout << "~~~~~~ ~~~~~~ Benchmark ~~~~~~ ~~~~~~" << std::endl;
            int payloadSize = test::Helpers::requestInt("(Int) payload size: ");

            std::string payload = "";
            for (int index = 0; index <= payloadSize; ++index) {
                payload += "0";
            }
            
            ttp2::Packet::Packet packet;
            ttp2::Packet::Standard standard;
            standard.payload = payload;
            packet.payload = standard;

            std::chrono::time_point start = std::chrono::steady_clock::now();
            int maxPackets = 0;
            int receivedPackets = 0;
            int lastPacketId = 0;
            while (true) {
                clientSessionController->pushRequest(packet);
                while(clientSessionController->hasResponse()) {
                    ttp2::Packet::Packet packet = clientSessionController->popResponse();
                    // Packet::Standard standard = std::get<Packet::Standard>(packet.payload);
                    lastPacketId = packet.id;
                    receivedPackets++;
                }
                
                std::chrono::time_point end = std::chrono::steady_clock::now();
                if (std::chrono::duration_cast<std::chrono::seconds>(end - start).count() >= 1) {
                    std::cout << "\r\033[2K" << "Last id: " << lastPacketId << " ~ " << receivedPackets << "pp/s" << " ~ max: " << maxPackets << "pp/s"<< std::flush;
                    start = end;
                    if (receivedPackets > maxPackets)
                        maxPackets = receivedPackets;
                    receivedPackets = 0;
                }
            }
        } else if (option == 4) {
          std::shared_ptr<arrow::Table> table = test::Helpers::openCsvFile();
          ttp2::Packet::Packet packet;
          ttp2::Packet::File file;
          file.start = 0;
          file.end = table->num_rows();
          file.payload = table;
          packet.payload = file;
          clientSessionController->pushRequest(packet);

          std::cout << "Done!" << std::endl;
        } else if (option == 5) {
          int responseQueueSize = clientSessionController->getResponseQueueSize();
          int index = 0;
          do {
              std::cout << "Peek index: 0 to " << responseQueueSize << " | -1 to exit" << std::endl;
              index = test::Helpers::requestInt("(int) index: ");
          } while (index < -1 || index > responseQueueSize);

          if (index == -1) {
              continue;
          }

          ttp2::ClientSessionController::PacketInfo packetInfo = clientSessionController->peekResponse(index);
          std::cout << "--- PacketInfo ---" << std::endl;
          std::string payloadType = "";
          if (std::holds_alternative<ttp2::Packet::Standard>(packetInfo.payloadType))
              payloadType = "Standard";
          else if (std::holds_alternative<ttp2::Packet::File>(packetInfo.payloadType))
              payloadType = "File";
          else if (std::holds_alternative<ttp2::Packet::Viewport>(packetInfo.payloadType))
              payloadType = "Viewport";
          else
              payloadType = "Invalid";
          
          std::cout << "id: " << packetInfo.id << "\npacketType: " << payloadType.c_str() << std::endl;
          std::cout << "---    ---     ---" << std::endl;
        } else if (option == 6) {
          std::shared_ptr<arrow::Table> table = test::Helpers::openCsvFile();
          ttp2::Packet::Packet packet;
          ttp2::Packet::Viewport viewport;
          viewport.xStart = 0;
          viewport.xEnd = table->num_rows();
          viewport.yStart = 0;
          viewport.yEnd = table->num_columns();
          viewport.payload = table;
          packet.payload = viewport;
          clientSessionController->pushRequest(packet);

          std::cout << "Done!" << std::endl;
        } else if (option == 7) {
          ttp2::Packet::Packet packet;
          ttp2::Packet::TqlQuery tqlQuery;
          std::string query = test::Helpers::requestString("Query >");
          tqlQuery.query = query;
          packet.payload = tqlQuery;
          clientSessionController->pushRequest(packet);
          std::cout << "Done!" << std::endl;
        } else if (option == 8) {
          ttp2::Packet::Packet packet;
          ttp2::Packet::Error error;
          error.message = "ERROR!";
          error.code = 404;
          packet.payload = error;
          clientSessionController->pushRequest(packet);
          std::cout << "Done!" << std::endl;
        } else if (option == 9) {
          int tdfsOption = test::Helpers::requestInt("(0) back\n(1) ls\n(2) ls Solution\noption:");

          if (tdfsOption == 0) {
            std::cout << "Back!" << std::endl;
          } else if (tdfsOption == 1) {
            ttp2::Packet::Packet packet;
            ttp2::Packet::tdfs::Ls ls;
            ls.directory = "/home/test";
            packet.payload = ls;
            clientSessionController->pushRequest(packet);
            std::cout << "Done!" << std::endl;
          } else if (tdfsOption == 2) {
            ttp2::Packet::Packet packet;
            ttp2::Packet::tdfs::LsSolution lsSolution;
            lsSolution.content = {"item1.txt", "item2.txt", "dir1/"};
            packet.payload = lsSolution;
            clientSessionController->pushRequest(packet);
            std::cout << "Done!" << std::endl;
          } else {
            std::cout << "Invalid!" << std::endl;
          }
        } else {
            std::cout << "Invalid!" << std::endl;
        }
    }

    std::cout << "Terminated!" << std::endl;
    
    networkThread.join();
  }
}
