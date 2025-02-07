#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <iomanip>

#include "csv.h"

static const int TWO_KB = 2048;

// Structure to accumulate key-based statistics.
struct KeyAgg {
    long long sumObjectSize = 0;
    long long count = 0;
};

// Structure to accumulate statistics while processing the CSV.
struct StatsAccumulator {
    long long totalKeySize = 0;
    long long totalValueSize = 0;
    long long totalObjectSize = 0;
    long long lineCount = 0;
    
    // Map from key to its aggregate (sum of object sizes and count)
    std::unordered_map<std::string, KeyAgg> mapKeyAgg;
};

// Structure to hold the final computed statistics.
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
// Update the accumulator with one CSV record.
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
    
    // Optional: print progress every 100M lines
    if (acc.lineCount % 100000000 == 0) {
        std::cout << "Processing " << acc.lineCount 
                  << " lines, remaining line: " 
                  << (61700000000LL - acc.lineCount) << "\n";
    }
    
    acc.lineCount++;
    
    // Update the key-based aggregation.
    auto &agg = acc.mapKeyAgg[key];
    agg.sumObjectSize += objectSize;
    agg.count++;
}

// ----------------------------------------------------------------
// Compute final statistics from an accumulator.
// ----------------------------------------------------------------
Stats computeStats(const StatsAccumulator &acc)
{
    Stats s;
    if (acc.lineCount == 0)
        return s;
    
    s.avgKeySize    = static_cast<double>(acc.totalKeySize)    / acc.lineCount;
    s.avgValueSize  = static_cast<double>(acc.totalValueSize)  / acc.lineCount;
    s.avgObjectSize = static_cast<double>(acc.totalObjectSize) / acc.lineCount;
    s.sumObjectSize = acc.totalObjectSize;
    
    long long sumKeyAverages = 0;
    for (const auto &kv : acc.mapKeyAgg) {
        const auto &agg = kv.second;
        if (agg.count > 0)
            sumKeyAverages += (agg.sumObjectSize / agg.count);
    }
    s.sumKeyBasedAvg = sumKeyAverages;
    s.uniqueKeyCount = acc.mapKeyAgg.size();
    s.totalKeyCount  = acc.lineCount;
    s.lineCount      = acc.lineCount;
    
    return s;
}

// ----------------------------------------------------------------
// Print the computed statistics.
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
    out << "  Total line count     : " << st.lineCount      << "\n\n";
}

int main(int argc, char* argv[]) {
    // Expecting two arguments: the input CSV file and the output file.
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
    
    // Prepare three accumulators:
    //   1) For all data.
    //   2) For records with object size <= 2KB.
    //   3) For records with object size > 2KB.
    StatsAccumulator accAll, accUnder2KB, accOver2KB;
    
    // ----------------------------------------------------------------
    // Create a CSVReader using fast-cpp-csv-parser.
    // We specify that we expect 5 columns.
    // The CSV file is expected to have a header row with:
    // "key", "op", "size", "op_count", "key_size"
    // io::ignore_extra_column ignores any additional columns.
    io::CSVReader<5> csvIn(traceFilePath);
    csvIn.read_header(io::ignore_extra_column, "key", "op", "size", "op_count", "key_size");
    
    // Variables to hold the CSV row fields.
    std::string key, op;
    int size, op_count, key_size;
    
    // Read each row and update the accumulators.
    while (csvIn.read_row(key, op, size, op_count, key_size)) {
        int objectSize = size;  // "size" is the object size.
        int valueSize = objectSize - key_size;
        
        // Update the "All" accumulator.
        updateStats(accAll, key, key_size, valueSize, objectSize);
        
        // Update the appropriate accumulator based on object size.
        if (objectSize <= TWO_KB)
            updateStats(accUnder2KB, key, key_size, valueSize, objectSize);
        else
            updateStats(accOver2KB, key, key_size, valueSize, objectSize);
    }
    
    // Compute final statistics.
    Stats statAll      = computeStats(accAll);
    Stats statUnder2KB = computeStats(accUnder2KB);
    Stats statOver2KB  = computeStats(accOver2KB);
    
    // Output the results.
    printStats(fout, statUnder2KB, "Under 2KB");
    printStats(fout, statOver2KB, "Over 2KB");
    printStats(fout, statAll, "All");
    
    fout.close();
    return 0;
}
