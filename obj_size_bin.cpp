#include <cmath>
#include <cassert>
#include "include/argparse/argparse.hpp"
#include "include/csv/csv.h"
#include "include/robin_hood/robin_hood.h"
#include <vector>

int main(int argc, char *argv[]) {
  argparse::ArgumentParser program("csv_analyzer", "1.0");

  program.add_argument("input_files")
      .help("One or more CSV trace files to analyze")
      .required()
      .remaining();

  try {
    program.parse_args(argc, argv);
  } catch (const std::runtime_error &err) {
    std::cerr << err.what() << std::endl;
    std::cerr << program;
    return 1;
  }

  std::vector<std::string> traceFiles;
  try {
    traceFiles = program.get<std::vector<std::string>>("input_files");
    if (traceFiles.empty()) {
      std::cerr << "No input files provided.\n";
      return 1;
    }
  } catch (...) {
    std::cerr << "No input files provided.\n";
    return 1;
  }

  uint32_t numBins = 0;
  uint32_t startByte = 64;
  uint32_t endByte = 512 * 1024;

  while (startByte <= endByte) {
    startByte *= 2;
    numBins++;
  }

  auto ceil_log2 = [](uint32_t val) {
    uint32_t floor_log = 31 - __builtin_clz(val);
    return (val == (1u << floor_log)) ? floor_log : floor_log + 1;
  };
  // Simple test
  assert(ceil_log2(64) == 6);
  assert(ceil_log2(62) == 6);

  std::vector<uint64_t> numObjs(numBins + 1, 0);

  for (const auto &filePath : traceFiles) {
    io::CSVReader<5> csvIn(filePath);
    csvIn.read_header(io::ignore_extra_column, "key", "op", "size", "op_count",
                      "key_size");

    std::string key, op;
    uint32_t size = 0, op_count = 0, key_size = 0;

    while (csvIn.read_row(key, op, size, op_count, key_size)) {
      uint32_t binIdx = ceil_log2(size) - 6;
      numObjs[std::min(binIdx, numBins)]++;
    }
  }

  for (std::size_t i = 0; i < numObjs.size(); ++i) {
    std::cout << 64 * static_cast<uint32_t>(std::pow(2, i)) << " " << numObjs[i] << std::endl;
  }

  return 0;
}
