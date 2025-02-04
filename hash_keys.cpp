#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <unordered_map>
#include <set>
#include "md5.h"

int main(int argc, char** argv)
{
    if (argc < 2) {
        std::cerr << "usage: " << argv[0] << " <trace_file>\n";
        return 1;
    }

    std::string traceFile = argv[1];
    std::ifstream ifs(traceFile);
    if (!ifs.is_open()) {
        std::cerr << "fail to open files: " << traceFile << "\n";
        return 1;
    }

    // Only unique keys
    std::set<std::string> uniqueKeys; 

    std::string line;
    while (std::getline(ifs, line)) {
        if (line.empty()) continue;

        auto pos = line.find(',');
        if (pos == std::string::npos) {
            uniqueKeys.insert(line);
        } else {
            std::string key = line.substr(0, pos);
            uniqueKeys.insert(key);
        }
    }
    ifs.close();

    for (int cutLen : {16, 17, 18}) {
        std::unordered_map<std::string, std::string> hashMap;
        bool collisionFound = false;

        for (auto &originalKey : uniqueKeys) {
            MD5 md5;
            md5.addData(originalKey.data(), originalKey.size());
            md5.finalize();

            std::string hash32 = md5.toString();
            std::string subHash = hash32.substr(0, cutLen);

            auto it = hashMap.find(subHash);
            if (it != hashMap.end()) {
                collisionFound = true;
                std::cout << "[Collision] MD5 " << cutLen << "hash collision!\n"
                          << "  - Hash value: " << subHash << "\n"
                          << "  - original key: " << it->second << "\n"
                          << "  - Hashed Key: " << originalKey << "\n\n";
            } else {
                hashMap[subHash] = originalKey;
            }
        }

        if (!collisionFound) {
            std::cout << cutLen << "No hash collision\n";
        }
    }

    return 0;
}
