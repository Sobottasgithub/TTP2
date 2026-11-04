#include "helpers.h"

#include <regex>
#include <iostream>

#include <arrow/csv/options.h>
#include <arrow/csv/api.h>
#include <arrow/io/api.h>
#include <filesystem>

namespace test {
  bool Helpers::isNumeric(const std::string& string) {
    static const std::regex numberRegex(
        R"(^[-+]?(?:\d+(?:\.\d*)?|\.\d+)(?:[eE][-+]?\d+)?$)"
    );
    return std::regex_match(string, numberRegex);
  }
  
  std::string Helpers::requestString(const std::string& message) {
    std::string userInput;
    std::cout << message << std::flush;
    std::getline(std::cin, userInput);
    return userInput;
  }
  
  int Helpers::requestInt(const std::string& message) {
    std::string userInput;

    while (true) {
        std::cout << message << std::flush;
        std::getline(std::cin, userInput);

        if (isNumeric(userInput)) {
            return std::stoi(userInput);
        }
        std::cout << "Invalid input! Try again" << std::endl;
    }
  }
  
  std::shared_ptr<arrow::Table> Helpers::openCsvFile() {
    std::string filePath { "" };
    do {
      if (filePath.length() > 0 && !std::filesystem::exists(filePath)) {
          std::cout << "Incorrect filepath!" << std::endl;
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
     std::cout << "Error while instantiating TableReader!" << std::endl;
    }
    std::shared_ptr<arrow::csv::TableReader> reader = *maybeReader;

    arrow::Result<std::shared_ptr<arrow::Table>> maybeTable = reader->Read();
    if (!maybeTable.ok()) {
      std::cout << "Error while read table from CSV file!" << std::endl;
    }
    std::shared_ptr<arrow::Table> table = *maybeTable;
    return table;
  }
}
