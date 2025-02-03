#include <fstream>
#include <iostream>
#include <random>
#include <string>

void sampleTraceFile(const std::string& inputFile, const std::string& outputFile, int n) {
    std::ifstream inFile(inputFile);
    std::ofstream outFile(outputFile);

    if (!inFile.is_open() || !outFile.is_open()) {
        std::cerr << "Error: Unable to open input or output file." << std::endl;
        return;
    }

    std::string header;
    if (std::getline(inFile, header)) {
        outFile << header << "\n"; // 헤더 유지
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> dist(0, n - 1);

    std::string line;
    std::vector<std::string> buffer;
    
    while (std::getline(inFile, line)) {
        buffer.push_back(line);
        if (buffer.size() == n) {
            int randIdx = dist(gen);  // 0 ~ (n-1) 중 랜덤으로 하나 선택
            outFile << buffer[randIdx] << "\n";  // 무조건 하나 출력
            buffer.clear();  // 버퍼 초기화
        }
    }

    // 마지막 남은 데이터가 있다면 한 개 랜덤 선택
    if (!buffer.empty()) {
        std::uniform_int_distribution<int> lastDist(0, buffer.size() - 1);
        outFile << buffer[lastDist(gen)] << "\n";
    }

    inFile.close();
    outFile.close();

    std::cout << "Sampling complete. Output saved to: " << outputFile << std::endl;
}
int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <input_file> <output_file> <n>" << std::endl;
        return 1;
    }
    
    std::string inputFile = argv[1];
    std::string outputFile = argv[2];
    int n = std::stoi(argv[3]);
    
    if (n <= 0) {
        std::cerr << "Error: n must be a positive integer." << std::endl;
        return 1;
    }
    
    sampleTraceFile(inputFile, outputFile, n);
    return 0;
}
