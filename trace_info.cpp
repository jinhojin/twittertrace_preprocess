#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <iomanip>

#include "csv.hpp"

static const int TWO_KB = 2048;

struct KeyAgg {
    long long sumObjectSize = 0;
    long long count = 0;
};

struct StatsAccumulator {
    long long totalKeySize = 0;
    long long totalValueSize = 0;
    long long totalObjectSize = 0;
    long long lineCount = 0;
    
    // For each unique key, store (sum of objectSize, number of occurrences)
    std::unordered_map<std::string, KeyAgg> mapKeyAgg;
};

struct Stats {
    double avgKeySize = 0.0;
    double avgValueSize = 0.0;
    double avgObjectSize = 0.0;
    long long sumObjectSize = 0;
    long long sumKeyBasedAvg = 0;
    
    long long uniqueKeyCount = 0;
    long long totalKeyCount = 0;
    long long lineCount = 0;
};

// ----------------------------------------------------------------
// Function to update the accumulator with a single record
// ----------------------------------------------------------------
inline void updateStats(StatsAccumulator &acc, 
                        const std::string &key, 
                        int keySize, 
                        int valueSize, 
                        int objectSize) 
{
    acc.totalKeySize    += keySize;
    acc.totalValueSize  += valueSize;
    acc.totalObjectSize += objectSize;
    
    // Optional: Print a progress message every 100M lines.
/*
    if (acc.lineCount % 100000000 == 0) {
        std::cout << "Processing " << acc.lineCount 
                  << " lines, remaining line: " 
                  << (61700000000LL - acc.lineCount) << "\n"; 
    }
*/
    acc.lineCount++;
    auto &agg = acc.mapKeyAgg[key];
    agg.sumObjectSize += objectSize;
    agg.count++;
}

// ----------------------------------------------------------------
// Function to compute final Stats from an accumulator
// ----------------------------------------------------------------
Stats computeStats(const StatsAccumulator &acc)
{
    Stats s;
    if (acc.lineCount == 0) {
        return s;  // no data, so return zeros
    }

    s.avgKeySize    = static_cast<double>(acc.totalKeySize)    / acc.lineCount;
    s.avgValueSize  = static_cast<double>(acc.totalValueSize)  / acc.lineCount;
    s.avgObjectSize = static_cast<double>(acc.totalObjectSize) / acc.lineCount;
    s.sumObjectSize = acc.totalObjectSize;

    long long sumKeyAverages = 0;
    for (const auto &kv : acc.mapKeyAgg) {
        const auto &agg = kv.second;
        if (agg.count > 0) {
            sumKeyAverages += (agg.sumObjectSize / agg.count);
        }
    }
    s.sumKeyBasedAvg = sumKeyAverages;
    s.uniqueKeyCount = acc.mapKeyAgg.size();
    s.totalKeyCount  = acc.lineCount;
    s.lineCount      = acc.lineCount;

    return s;
}

// ----------------------------------------------------------------
// Function to print stats to the given output stream
// ----------------------------------------------------------------
void printStats(std::ostream &out, const Stats &st, const std::string &title) {
    out << "=== " << title << " ===\n";
    out << "  Average key size     : " << std::fixed << std::setprecision(2) << st.avgKeySize << "\n";
    out << "  Average value size   : " << std::fixed << std::setprecision(2) << st.avgValueSize << "\n";
    out << "  Average object size  : " << std::fixed << std::setprecision(2) << st.avgObjectSize << "\n";
    out << "  Footprint1 (sum of object size)               : " << st.sumObjectSize << "\n";
    out << "  Footprint2 (sum of average of duplicated key) : " << st.sumKeyBasedAvg << "\n";
    out << "  Unique key count     : " << st.uniqueKeyCount << "\n";
    out << "  Total key count      : " << st.totalKeyCount  << "\n";
    out << "  Total line count     : " << st.lineCount      << "\n";
    out << std::endl;
}

int main(int argc, char* argv[]) {
    // Check command-line arguments.
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <trace_file> <output_file>\n";
        return 1;
    }
    const char* traceFilePath = argv[1];
    const char* outputFilePath = argv[2];

    // Open the output file.
    std::ofstream fout(outputFilePath);
    if (!fout.is_open()) {
        std::cerr << "Cannot open output file: " << outputFilePath << "\n";
        return 1;
    }

    // ----------------------------------------------------------------
    // Setup CSV parser using vincentlaucsb/csv-parser.
    // ----------------------------------------------------------------
    csv::CSVFormat format;
    format.delimiter(','); // comma is the default delimiter

   
    csv::CSVReader reader(traceFilePath, format);

    StatsAccumulator accAll, accUnder2KB, accOver2KB;

    // ----------------------------------------------------------------
    // Process each row in the CSV file.
    // ----------------------------------------------------------------
    for (csv::CSVRow &row : reader) {
        // Access columns by their header names.
        std::string key      = row["key"].get<>();
        std::string op       = row["op"].get<>();  // op is read but not used in calculations
        int objectSize       = row["size"].get<int>();      // "size" column
        int opCount          = row["op_count"].get<int>();    // "op_count" column (unused)
        int keySize          = row["key_size"].get<int>();    // "key_size" column

        int valueSize = objectSize - keySize;

        // Update the "All" accumulator.
        updateStats(accAll, key, keySize, valueSize, objectSize);

        // Update accumulators based on object size.
        if (objectSize <= TWO_KB) {
            updateStats(accUnder2KB, key, keySize, valueSize, objectSize);
        } else {
            updateStats(accOver2KB, key, keySize, valueSize, objectSize);
        }
    }

    // ----------------------------------------------------------------
    // Compute final statistics for each accumulator.
    // ----------------------------------------------------------------
    Stats statAll      = computeStats(accAll);
    Stats statUnder2KB = computeStats(accUnder2KB);
    Stats statOver2KB  = computeStats(accOver2KB);

    // ----------------------------------------------------------------
    // Output the results to the output file.
    // ----------------------------------------------------------------
    printStats(fout, statUnder2KB, "Under 2KB");
    printStats(fout, statOver2KB,  "Over 2KB");
    printStats(fout, statAll,      "All");

    fout.close();
    return 0;
}
