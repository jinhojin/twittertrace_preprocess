#include "include/argparse/argparse.hpp"
#include "include/csv/csv.hpp"
#include <chrono>
#include <filesystem>
#include <format>
#include <fstream>
#include <vector>

int main(int argc, char **argv) {
  argparse::ArgumentParser options("parser");

  options.add_argument("-i", "--input")
      .required()
      .help("Specify the input file to be split");
  options.add_argument("-o", "--output")
      .required()
      .help("Specify the output name");
  options.add_argument("-l", "--lines")
      .required()
      .scan<'u', uint32_t>()
      .help("Specify the number of lines for each file except the header");

  try {
    options.parse_args(argc, argv);
  } catch (const std::exception &err) {
    std::cerr << err.what() << std::endl;
    std::cerr << options;
    std::exit(1);
  }

  auto traceFilePath = options.get<std::string>("--input");
  if (!std::filesystem::exists(traceFilePath)) {
    std::cerr << std::format("Trace file: {} does not exist", traceFilePath)
              << std::endl;
    std::exit(1);
  }
  csv::CSVReader reader(traceFilePath);

  // Header: key,op,size,op_count,key_size
  auto outputPrefix = options.get<std::string>("--output");
  std::ofstream output;

  uint64_t numLines = 0;
  uint64_t targetNumLines = options.get<uint32_t>("--lines");

  // To check throughput
  auto start = std::chrono::high_resolution_clock::now();
  for (const auto &row : reader) {
    if (numLines % targetNumLines == 0) {
      // Calculate throughput
      auto end = std::chrono::high_resolution_clock::now();
      std::chrono::duration<double> elapsed = end - start;
      std::cout << std::format("Processing throughput: {:.2f} / sec (total {} "
                               "lines are processed)",
                               numLines / elapsed.count(), numLines)
                << std::endl;

      output.close();
      auto outputFileName =
          std::format("./{}_{}.csv", outputPrefix, numLines / targetNumLines);
      output.open(outputFileName, std::ios::out);
      // Write header for each split file
      output << "key,op,size,op_count,key_size" << "\n";
    }
    output << std::format("{},{},{},{},{}", row["key"].get<std::string>(),
                          row["op"].get<std::string>(),
                          row["size"].get<std::string>(),
                          row["op_count"].get<std::string>(),
                          row["key_size"].get<std::string>())
           << "\n";
    numLines++;
  }

  std::cout << std::format("total processed lines: {}", numLines) << std::endl;
  return 0;
}
