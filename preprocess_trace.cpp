#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

struct Row {
    uint64_t timestamp;
    std::string key;
    uint32_t key_size;
    uint32_t value_size;
    uint64_t client_id;
    std::string operation;
    uint64_t TTL;
};

// Helper function to fix the key field by removing commas
std::string fixKey(const std::vector<std::string>& fields, size_t startIdx, size_t endIdx) {
    std::string fixedKey;
    for (size_t i = startIdx; i <= endIdx; ++i) {
        fixedKey += fields[i];
    }
    return fixedKey;
}

// Function to parse a line into a Row structure
bool parseLine(const std::string& line, Row& row) {
    std::istringstream ss(line);
    std::string token;
    std::vector<std::string> fields;

    // Split line into tokens
    while (std::getline(ss, token, ',')) {
        fields.push_back(token);
    }

    // Ensure at least 7 fields exist
    if (fields.size() < 7) {
        return false; // Invalid row
    }

    // The key field may span multiple tokens
    size_t keyEndIdx = fields.size() - 6;

    try {
        row.timestamp = std::stoull(fields[0]);
        row.key = fixKey(fields, 1, keyEndIdx);
        row.key_size = std::stoul(fields[keyEndIdx + 1]);
        row.value_size = std::stoul(fields[keyEndIdx + 2]);
        row.client_id = std::stoull(fields[keyEndIdx + 3]);
        row.operation = fields[keyEndIdx + 4];
        row.TTL = std::stoull(fields[keyEndIdx + 5]);
    } catch (...) {
        return false; // Conversion error
    }

    return true;
}

// Function to process the CSV file
void processCSV(const std::string& inputFile, const std::string& outputFile) {
    std::ifstream inFile(inputFile);
    std::ofstream outFile(outputFile);

    if (!inFile.is_open() || !outFile.is_open()) {
        std::cerr << "Failed to open input or output file." << std::endl;
        return;
    }

    std::string line;
    while (std::getline(inFile, line)) {
        Row row;
        if (!parseLine(line, row)) {
            continue; // Skip invalid rows
        }

        // Filtering conditions
        if (row.operation != "get" && row.operation != "gets" && row.operation != "delete") {
            continue; // Remove rows with invalid operation
        }

        if ((row.operation == "get" || row.operation == "gets") && row.value_size == 0) {
            continue; // Remove rows with get/gets and value_size == 0
        }

        // Write the processed row to the output file
        outFile << row.timestamp << "," << row.key << "," << row.key_size << ","
                << row.value_size << "," << row.client_id << "," << row.operation << ","
                << row.TTL << "\n";
    }

    inFile.close();
    outFile.close();
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <input_file> <output_file>" << std::endl;
        return 1;
    }

    std::string inputFile = argv[1];
    std::string outputFile = argv[2];

    processCSV(inputFile, outputFile);

    std::cout << "CSV processing completed. Processed data saved to " << outputFile << std::endl;
    return 0;
}
