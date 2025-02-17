#include <iostream>
#include <fstream>
#include <string>
#include <iomanip>
#include <vector>

#include "include/robin_hood/robin_hood.h"
#include "include/csv/csv.h"
#include "include/argparse/argparse.hpp"

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
    
    robin_hood::unordered_map<std::string, KeyAgg> mapKeyAgg;
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
// Read oneline and update
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
    
    if (acc.lineCount > 0 && acc.lineCount % 100000000 == 0) {
        std::cout << "Processing " << acc.lineCount 
                  << " lines, maybe more...\n";
    }
    acc.lineCount++;
    
    auto &agg = acc.mapKeyAgg[key];
    agg.sumObjectSize += objectSize;
    agg.count++;
}

// ----------------------------------------------------------------
// StatsAccumulator => Stats
// ----------------------------------------------------------------
Stats computeStats(const StatsAccumulator &acc)
{
    Stats s;
    if (acc.lineCount == 0) {
        return s;
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
// Print stats
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
    argparse::ArgumentParser program("csv_analyzer", "1.0");
    
    program.add_argument("-o", "--output")
        .required()
        .help("Output file path");
    
    program.add_argument("input_files")
        .help("One or more CSV trace files to analyze")
        .remaining();
    
    try {
        program.parse_args(argc, argv);
    } catch (const std::runtime_error &err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        return 1;
    }

    auto outputFilePath = program.get<std::string>("--output");
    
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
    
    std::ofstream fout(outputFilePath);
    if (!fout.is_open()) {
        std::cerr << "Cannot open output file: " << outputFilePath << "\n";
        return 1;
    }
    
    StatsAccumulator accAll, accUnder2KB, accOver2KB;
    
    for (const auto &filePath : traceFiles) {
        io::CSVReader<5> csvIn(filePath);
        csvIn.read_header(io::ignore_extra_column, 
                          "key", "op", "size", "op_count", "key_size");
        
        std::string key, op;
        int size = 0, op_count = 0, key_size = 0;
        
        while (csvIn.read_row(key, op, size, op_count, key_size)) {
            int objectSize = size;  
            int valueSize = objectSize - key_size;
            
            updateStats(accAll,      key, key_size, valueSize, objectSize);
            if (objectSize <= TWO_KB) {
                updateStats(accUnder2KB, key, key_size, valueSize, objectSize);
            } else {
                updateStats(accOver2KB,  key, key_size, valueSize, objectSize);
            }
        }
    }
    
    Stats statAll      = computeStats(accAll);
    Stats statUnder2KB = computeStats(accUnder2KB);
    Stats statOver2KB  = computeStats(accOver2KB);
    
    printStats(fout, statUnder2KB, "Under 2KB");
    printStats(fout, statOver2KB,  "Over 2KB");
    printStats(fout, statAll,      "All");
    
    fout.close();
    return 0;
}
