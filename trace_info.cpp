#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <iomanip>

static const int TWO_KB = 2048;

struct KeyAgg {
    long long sumObjectSize = 0;
    long long count         = 0;
};

struct StatsAccumulator {
    long long totalKeySize    = 0;
    long long totalValueSize  = 0;
    long long totalObjectSize = 0;
    long long lineCount       = 0;

    // For each unique key, store (sum of objectSize, number of occurrences)
    std::unordered_map<std::string, KeyAgg> mapKeyAgg;
};

struct Stats {
    double avgKeySize       = 0.0;
    double avgValueSize     = 0.0;
    double avgObjectSize    = 0.0;
    long long sumObjectSize = 0;
    long long sumKeyBasedAvg= 0;
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
    acc.lineCount++;

    // Update key-based accumulation
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
        // If no data, return zero-initialized stats
        return s;
    }

    // Calculate averages
    s.avgKeySize    = static_cast<double>(acc.totalKeySize)    / acc.lineCount;
    s.avgValueSize  = static_cast<double>(acc.totalValueSize)  / acc.lineCount;
    s.avgObjectSize = static_cast<double>(acc.totalObjectSize) / acc.lineCount;

    // Total object size (footprint1)
    s.sumObjectSize = acc.totalObjectSize;

    // Sum of average object size for each key (footprint2)
    long long sumKeyAverages = 0;
    for (const auto &kv : acc.mapKeyAgg) {
        const auto &agg = kv.second;
        if (agg.count > 0) {
            sumKeyAverages += (agg.sumObjectSize / agg.count);
        }
    }
    s.sumKeyBasedAvg = sumKeyAverages;

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
    out << std::endl;
}

int main(int argc, char* argv[]) {
    // We need 2 arguments: input trace file and output file
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <trace_file> <output_file>\n";
        return 1;
    }

    // Open the input trace file
    const char* traceFilePath = argv[1];
    std::ifstream fin(traceFilePath);
    if (!fin.is_open()) {
        std::cerr << "Cannot open input file: " << traceFilePath << "\n";
        return 1;
    }

    // Open the output file
    const char* outputFilePath = argv[2];
    std::ofstream fout(outputFilePath);
    if (!fout.is_open()) {
        std::cerr << "Cannot open output file: " << outputFilePath << "\n";
        return 1;
    }

    // Prepare three accumulators:
    // 1) All data
    // 2) Under (or equal to) 2KB
    // 3) Over 2KB
    StatsAccumulator accAll, accUnder2KB, accOver2KB;

    // --------------------------------------------------
    // (1) Optional: Skip header if present
    // --------------------------------------------------
    {
        std::string headerLine;
        if (std::getline(fin, headerLine)) {
            // If the line starts with "key,op,size,op_count,key_size", assume it's a header
            // Otherwise, parse it as a data line
            if (headerLine.rfind("key,op,size,op_count,key_size", 0) != 0) {
                // Not a header -> parse it right away
                std::stringstream ss(headerLine);
                std::string key, op;
                std::string objectSizeStr, opCountStr, keySizeStr;
                if (std::getline(ss, key, ',') &&
                    std::getline(ss, op, ',') &&
                    std::getline(ss, objectSizeStr, ',') &&
                    std::getline(ss, opCountStr, ',') &&
                    std::getline(ss, keySizeStr, ','))
                {
                    int objectSize  = std::stoi(objectSizeStr);
                    int opCount     = std::stoi(opCountStr);
                    int keySize     = std::stoi(keySizeStr);
                    int valueSize   = objectSize - keySize;

                    // Update accumulators
                    updateStats(accAll, key, keySize, valueSize, objectSize);
                    if (objectSize <= TWO_KB) {
                        updateStats(accUnder2KB, key, keySize, valueSize, objectSize);
                    } else {
                        updateStats(accOver2KB, key, keySize, valueSize, objectSize);
                    }
                }
            }
            // If it's a real header, do nothing (skip the line)
        }
    }

    // --------------------------------------------------
    // (2) Read each line, parse, and accumulate
    // --------------------------------------------------
    std::string line;
    while (true) {
        if (!std::getline(fin, line)) {
            break;
        }
        if (line.empty()) {
            continue;
        }

        std::stringstream ss(line);
        std::string key, op;
        std::string objectSizeStr, opCountStr, keySizeStr;
        if (!std::getline(ss, key, ','))     continue;
        if (!std::getline(ss, op, ','))      continue;
        if (!std::getline(ss, objectSizeStr, ',')) continue;
        if (!std::getline(ss, opCountStr, ','))    continue;
        if (!std::getline(ss, keySizeStr, ','))    continue;

        int objectSize = std::stoi(objectSizeStr);
        int keySize    = std::stoi(keySizeStr);
        int valueSize  = objectSize - keySize;

        // Update 'All' accumulator
        updateStats(accAll, key, keySize, valueSize, objectSize);

        // Determine which group (under2KB or over2KB)
        if (objectSize <= TWO_KB) {
            updateStats(accUnder2KB, key, keySize, valueSize, objectSize);
        } else {
            updateStats(accOver2KB, key, keySize, valueSize, objectSize);
        }
    }
    fin.close();

    // --------------------------------------------------
    // (3) Compute final statistics
    // --------------------------------------------------
    Stats statAll      = computeStats(accAll);
    Stats statUnder2KB = computeStats(accUnder2KB);
    Stats statOver2KB  = computeStats(accOver2KB);

    // --------------------------------------------------
    // (4) Output the results to the output file
    // --------------------------------------------------
    printStats(fout, statUnder2KB, "Under 2KB");
    printStats(fout, statOver2KB,  "Over 2KB");
    printStats(fout, statAll,      "All");

    // You can also print to the console if desired:
    // printStats(std::cout, statUnder2KB, "Under 2KB");
    // printStats(std::cout, statOver2KB,  "Over 2KB");
    // printStats(std::cout, statAll,      "All");

    fout.close();

    return 0;
}
