#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <fstream>
#include <sstream>

#include "robin_hood.h"
#include "csv.h"
#include "md5.h"   


std::string md5Truncate(const std::string &input, size_t length) {
    Chocobo1::MD5 md5;
    md5.addData(input.data(), input.size()).finalize();
    std::string hashHex = md5.toString();
    
    return hashHex.substr(0, length);

}

int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cerr << "[Usage] " << argv[0] << " <input_csv_file>\n";
        return 1;
    }

    const std::string inputCsvFile = argv[1];
    
    std::unordered_set<std::string> uniqueKeys;
    
    try {
        io::CSVReader<5> in(inputCsvFile);
        in.read_header(io::ignore_extra_column,
                       "key", "op", "size", "op_count", "key_size");
        size_t lineCount = 0; 
        std::string key, op;
        int size, op_count, key_size;
        
        while (in.read_row(key, op, size, op_count, key_size)) {
            lineCount++;
	    if(lineCount % 10000000 == 0) {
	       std::cout << "Processed " << lineCount << " lines so far...\n";
	    }
	    uniqueKeys.insert(key);
        }
    } catch (const io::error::base& e) {
        std::cerr << "CSV parsing error: " << e.what() << "\n";
        return 1;
    }
    
    std::vector<size_t> testLengths = {16, 17, 18};

    for (auto len : testLengths) {
        std::cout << "\n[ Checking MD5 hash collision for length = " << len << " ]\n";
        
        robin_hood::unordered_map<std::string, std::string> hashMap;
        
        bool conflictFound = false;

        for (const auto &origKey : uniqueKeys) {
            std::string hashedKey = md5Truncate(origKey, len);
            
            auto it = hashMap.find(hashedKey);
            if (it != hashMap.end()) {
                conflictFound = true;
                std::cout << "Conflict detected! HashedKey='" << hashedKey
                          << "'\n - Existing Original Key: " << it->second
                          << "\n - New Original Key:      " << origKey << "\n\n";
            } else {
                hashMap[hashedKey] = origKey;
            }
        }

        if (!conflictFound) {
            std::cout << "No conflicts for hash length = " << len << "\n";
        }
    }

    return 0;
}
