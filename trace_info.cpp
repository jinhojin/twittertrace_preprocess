#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <iomanip> // std::setprecision

struct TraceLine {
    std::string key;
    std::string op;
    int objectSize;
    int opCount;
    int keySize;
    int valueSize;
};

struct Stats {
    double avgKeySize       = 0.0;  
    double avgValueSize     = 0.0;
    double avgObjectSize    = 0.0;
    long long sumObjectSize = 0;   
    long long sumKeyBasedAvg = 0;  
};

static const int TWO_KB = 2048;

Stats computeStats(const std::vector<TraceLine>& lines) {
    Stats s;
    if (lines.empty()) {
        return s; 
    }
    long long totalKeySize     = 0;
    long long totalValueSize   = 0;
    long long totalObjectSize  = 0;

    std::map<std::string, std::pair<long long, int>> keyToObjectSumCount;
    for (auto &line : lines) {
        totalKeySize    += line.keySize;
        totalValueSize  += line.valueSize;
        totalObjectSize += line.objectSize;

        auto &entry = keyToObjectSumCount[line.key];
        entry.first  += line.objectSize;
        entry.second += 1;
    }

    int n = static_cast<int>(lines.size());
    s.avgKeySize    = static_cast<double>(totalKeySize) / n;
    s.avgValueSize  = static_cast<double>(totalValueSize) / n;
    s.avgObjectSize = static_cast<double>(totalObjectSize) / n;
    s.sumObjectSize = totalObjectSize;

    long long sumKeyAverages = 0;
    for (auto &kv : keyToObjectSumCount) {
        long long objectSum = kv.second.first;
        int       count     = kv.second.second;
        long long avgObjForKey = (count > 0) ? (objectSum / count) : 0;
        sumKeyAverages += avgObjForKey;
    }
    s.sumKeyBasedAvg = sumKeyAverages;

    return s;
}

void printStats(const Stats &st, const std::string &title) {
    std::cout << "=== " << title << " ===\n";
    std::cout << "  Average key size     : " << std::fixed << std::setprecision(2) << st.avgKeySize << "\n";
    std::cout << "  Average value size   : " << std::fixed << std::setprecision(2) << st.avgValueSize << "\n";
    std::cout << "  Average object size  : " << std::fixed << std::setprecision(2) << st.avgObjectSize << "\n";
    std::cout << "  Footprint1, sum of object size                 : " << st.sumObjectSize << "\n";
    std::cout << "  Footprint2, sum of average of duplicated key   : " << st.sumKeyBasedAvg << "\n";
    std::cout << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "usage: " << argv[0] << " <trace_file>\n";
        return 1;
    }

    const char* traceFilePath = argv[1];
    std::ifstream fin(traceFilePath);
    if (!fin.is_open()) {
        std::cerr << "cannot open file: " << traceFilePath << "\n";
        return 1;
    }

    std::vector<TraceLine> allLines;

    // =============================================
    // (1) Skip if header
    // =============================================
    {
        std::string headerLine;
        if (std::getline(fin, headerLine)) {
            if (headerLine.rfind("key,op,size,op_count,key_size", 0) != 0) {
                std::stringstream ss(headerLine);
                std::string key, op, objectSizeStr, opCountStr, keySizeStr;
                if (std::getline(ss, key, ',') &&
                    std::getline(ss, op, ',') &&
                    std::getline(ss, objectSizeStr, ',') &&
                    std::getline(ss, opCountStr, ',') &&
                    std::getline(ss, keySizeStr, ',')) 
                {
                    TraceLine t;
                    t.key         = key;
                    t.op          = op;
                    t.objectSize  = std::stoi(objectSizeStr);
                    t.opCount     = std::stoi(opCountStr);
                    t.keySize     = std::stoi(keySizeStr);
                    t.valueSize   = t.objectSize - t.keySize;
                    allLines.push_back(t);
                }
            }
        }
    }

    std::string line;
    // =============================================
    // (2) Read the rest of file
    // =============================================
    while (true) {
        if (!std::getline(fin, line)) {
            break;
        }
        if (line.empty()) {
            continue;
        }

        std::stringstream ss(line);
        std::string key, op, objectSizeStr, opCountStr, keySizeStr;
        if (!std::getline(ss, key, ',')) continue;
        if (!std::getline(ss, op, ',')) continue;
        if (!std::getline(ss, objectSizeStr, ',')) continue;
        if (!std::getline(ss, opCountStr, ',')) continue;
        if (!std::getline(ss, keySizeStr, ',')) continue;

        TraceLine t;
        t.key         = key;
        t.op          = op;
        t.objectSize  = std::stoi(objectSizeStr);
        t.opCount     = std::stoi(opCountStr);
        t.keySize     = std::stoi(keySizeStr);
        t.valueSize   = t.objectSize - t.keySize;

        allLines.push_back(t);
    }

    fin.close();

    // 2KB 이하 / 2KB 초과 구분
    std::vector<TraceLine> underEq2KB;
    std::vector<TraceLine> over2KB;
    for (auto &tl : allLines) {
        if (tl.objectSize <= TWO_KB) {
            underEq2KB.push_back(tl);
        } else {
            over2KB.push_back(tl);
        }
    }

    // 통계 계산
    Stats statUnder2KB = computeStats(underEq2KB);
    Stats statOver2KB  = computeStats(over2KB);
    Stats statAll      = computeStats(allLines);

    // 결과 출력
    printStats(statUnder2KB, "Under 2KB" );
    printStats(statOver2KB,  "Over 2KB");
    printStats(statAll,      "All");

    return 0;
}
