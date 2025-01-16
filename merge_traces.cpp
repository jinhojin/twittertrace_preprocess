#include <fstream>
#include <iostream>
#include <memory>
#include <queue>
#include <random>
#include <string>
#include <unordered_set>
#include <vector>

#include "csv.h"

struct TraceEntry {
  uint64_t timestamp;
  std::string key;
  uint32_t key_size;
  uint32_t value_size;
  uint64_t client_id;
  std::string operation;
  uint64_t TTL;
  size_t fileIndex;

  bool operator<(const TraceEntry& other) const {
    return timestamp > other.timestamp;
  }
};

void mergeAndTransformCsv(const std::vector<std::string>& inputFiles,
                          const std::string& outputFile,
                          int n,
                          bool includeSetOps) {
  std::vector<std::unique_ptr<io::CSVReader<7>>> readers;
  std::priority_queue<TraceEntry> minHeap;

  std::random_device rd;
  std::mt19937 gen(rd());

  // operation filtering
  std::unordered_set<std::string> defaultOps = {"get", "gets", "delete"};
  std::unordered_set<std::string> extendedOps = {
      "set", "cas", "add", "replace", "incr", "decr", "prepend", "append"};

  for (size_t i = 0; i < inputFiles.size(); ++i) {
    auto reader = std::make_unique<io::CSVReader<7>>(inputFiles[i]);

    TraceEntry entry;

    int lineCount = 0;
    std::vector<TraceEntry> group;

    // TODO: ignore columns: client_id, TTL
    while (reader->read_row(entry.timestamp,
                            entry.key,
                            entry.key_size,
                            entry.value_size,
                            entry.client_id,
                            entry.operation,
                            entry.TTL)) {
      lineCount++;

      // Filter operations first
      if (defaultOps.count(entry.operation) == 0 &&
          (!includeSetOps || extendedOps.count(entry.operation) == 0)) {
        continue; // Skip invalid operations
      }

      group.push_back(entry); // Add valid row to the group

      // Process group when it reaches `n` size
      if (group.size() == n) {
        std::uniform_int_distribution<> dis(0, group.size() - 1);
        minHeap.push(group[dis(gen)]);
        group.clear(); // Clear group for next batch
      }
    }

    // Process remaining rows in the last group
    if (!group.empty()) {
      std::uniform_int_distribution<> dis(0, group.size() - 1);
      minHeap.push(group[dis(gen)]);
    }

    readers.push_back(std::move(reader));
  }

  // make output file
  std::ofstream outFile(outputFile);
  if (!outFile.is_open()) {
    std::cerr << "Failed to open output file: " << outputFile << std::endl;
    return;
  }

  // using priority queue
  while (!minHeap.empty()) {
    TraceEntry smallest = minHeap.top();
    minHeap.pop();

    std::string opType;
    if (smallest.operation == "get" || smallest.operation == "gets") {
      opType = "GET";
    } else if (smallest.operation == "delete") {
      opType = "DELETE";
    } else if (includeSetOps && extendedOps.count(smallest.operation) > 0) {
      opType = "SET";
    } else {
      continue; // ignore
    }

    int size = smallest.key_size + smallest.value_size;
	outFile << smallest.timestamp << "," << smallest.key << ","
        << smallest.key_size << "," << smallest.value_size << ","
        << smallest.client_id << "," << smallest.operation << ","
        << smallest.TTL << "\n";

    size_t fileIndex = smallest.fileIndex;
    TraceEntry nextEntry;
    if (readers[fileIndex]->read_row(nextEntry.timestamp,
                                     nextEntry.key,
                                     nextEntry.key_size,
                                     nextEntry.value_size,
                                     nextEntry.client_id,
                                     nextEntry.operation,
                                     nextEntry.TTL)) {
      nextEntry.fileIndex = fileIndex;

      //operation filtering 
      if (defaultOps.count(nextEntry.operation) > 0 ||
          (includeSetOps && extendedOps.count(nextEntry.operation) > 0)) {
        minHeap.push(nextEntry);
      }
    }
  }

  outFile.close();
  std::cout << "Merged trace saved to: " << outputFile << std::endl;
}

int main(int argc, char* argv[]) {
  // TODO: use argparse.h
  if (argc < 4) {
    std::cerr << "Usage: " << argv[0]
              << " output_file n [--include-set-ops] input_file1 [input_file2 "
                 "... input_filen]\n";
    return 1;
  }

  std::string outputFile = argv[1];
  int n = std::stoi(argv[2]);
  if (n <= 0) {
    std::cerr << "n must be a positive integer.\n";
    return 1;
  }

  bool includeSetOps = false;
  int inputStartIndex = 3;

  // process set ops
  if (std::string(argv[3]) == "--include-set-ops") {
    includeSetOps = true;
    inputStartIndex++;
  }

  std::vector<std::string> inputFiles(argv + inputStartIndex, argv + argc);

  mergeAndTransformCsv(inputFiles, outputFile, n, includeSetOps);

  return 0;
}
