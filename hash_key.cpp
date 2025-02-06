#include <iostream>
#include <fstream>
#include <string>
#include "csv.h"       
#include "md5.h"       

std::string md5Truncate(const std::string& input, size_t length = 16) {
    Chocobo1::MD5 md5;
    md5.addData(input.data(), input.size()).finalize();
    std::string hashHex = md5.toString(); 
    if (hashHex.size() > length) {
        return hashHex.substr(0, length);
    }
    return hashHex;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <input_csv> <output_csv>\n";
        return 1;
    }

    const std::string inputCsv = argv[1];
    const std::string outputCsv = argv[2];

    io::CSVReader<5> in(inputCsv);
    in.read_header(io::ignore_extra_column, 
                   "key", "op", "size", "op_count", "key_size");

    std::ofstream out(outputCsv);
    if (!out.is_open()) {
        std::cerr << "Failed to open output file: " << outputCsv << std::endl;
        return 1;
    }

    out << "key,op,size,op_count,key_size\n";

    std::string key, op;
    int size, op_count, key_size;
    size_t lineCount = 0;
    while (in.read_row(key, op, size, op_count, key_size)) {
        std::string newKey = md5Truncate(key, 16);
	if(lineCount%100000000){
            std::cout << "processed line: " << lineCount << " remaining line: " << 61700000000-lineCount << "\n";
        }
	lineCount++;
	out << newKey << "," 
            << op << "," 
            << size << "," 
            << op_count << "," 
            << key_size 
            << "\n";
    }

    out.close();
    std::cout << "Done! Created file: " << outputCsv << std::endl;

    return 0;
}
