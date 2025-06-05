#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <ctime>
#include <array>
#include <iomanip>

enum SingleBitHistory { NotTaken = 0, Taken };
enum TwoBitSaturatingHistory { StronglyTaken = 0, WeaklyTaken, WeaklyNotTaken, StronglyNotTaken };
enum PredictorSelector { StronglyGShare = 0, WeaklyGShare, WeaklyBimodal, StronglyBimodal };

// 11-bit global history register packed
struct GlobalHistoryRegister {
    unsigned int bits : 11;
};

// Function declarations
void alwaysTakenPredictor(const std::vector<std::string>& traces, std::ofstream& output);
void neverTakenPredictor(const std::vector<std::string>& traces, std::ofstream& output);
void singleBitBimodalPredictor(const std::vector<std::string>& traces, std::ofstream& output);
void twoBitBimodalPredictor(const std::vector<std::string>& traces, std::ofstream& output);
void gsharePredictor(const std::vector<std::string>& traces, std::ofstream& output);
void tournamentPredictor(const std::vector<std::string>& traces, std::ofstream& output);

// Hardcoded branch outcome positions
constexpr size_t OUTCOME_INDEX = 11;
constexpr char TAKEN_CHAR = 'T';
constexpr char NOT_TAKEN_CHAR = 'N';

void alwaysTakenPredictor(const std::vector<std::string>& traces, std::ofstream& output) {
    long correct = 0;
    for (const auto& trace : traces) {
        if (trace[OUTCOME_INDEX] == TAKEN_CHAR) correct++;
    }
    output << correct << "," << traces.size() << ";\n";
    std::cout << correct << "," << traces.size() << ";\n";
}

void neverTakenPredictor(const std::vector<std::string>& traces, std::ofstream& output) {
    long correct = 0;
    for (const auto& trace : traces) {
        if (trace[OUTCOME_INDEX] == NOT_TAKEN_CHAR) correct++;
    }
    output << correct << "," << traces.size() << ";\n";
    std::cout << correct << "," << traces.size() << ";\n";
}

void singleBitBimodalPredictor(const std::vector<std::string>& traces, std::ofstream& output) {
    for (int size = 16; size <= 2048; size *= (size == 32 ? 4 : 2)) {
        std::vector<SingleBitHistory> table(size, Taken);
        long correct = 0;

        for (const auto& trace : traces) {
            long address = std::stoul(trace.substr(0, 10), nullptr, 16);
            size_t index = address % size;

            if (table[index] == Taken && trace[OUTCOME_INDEX] == TAKEN_CHAR) {
                correct++;
            } else if (table[index] == NotTaken && trace[OUTCOME_INDEX] == NOT_TAKEN_CHAR) {
                correct++;
            } else {
                table[index] = (trace[OUTCOME_INDEX] == TAKEN_CHAR) ? Taken : NotTaken;
            }
        }
        output << correct << "," << traces.size() << (size == 2048 ? ";\n" : "; ");
        std::cout << correct << "," << traces.size() << (size == 2048 ? ";\n" : "; ");
    }
}

void twoBitBimodalPredictor(const std::vector<std::string>& traces, std::ofstream& output) {
    for (int size = 16; size <= 2048; size *= (size == 32 ? 4 : 2)) {
        std::vector<TwoBitSaturatingHistory> table(size, StronglyTaken);
        long correct = 0;

        for (const auto& trace : traces) {
            long address = std::stoul(trace.substr(0, 10), nullptr, 16);
            size_t index = address % size;
            auto& entry = table[index];

            bool isTaken = (trace[OUTCOME_INDEX] == TAKEN_CHAR);

            switch (entry) {
                case StronglyTaken:
                    correct += isTaken;
                    entry = isTaken ? StronglyTaken : WeaklyTaken;
                    break;
                case WeaklyTaken:
                    correct += isTaken;
                    entry = isTaken ? StronglyTaken : WeaklyNotTaken;
                    break;
                case WeaklyNotTaken:
                    correct += !isTaken;
                    entry = isTaken ? WeaklyTaken : StronglyNotTaken;
                    break;
                case StronglyNotTaken:
                    correct += !isTaken;
                    entry = isTaken ? WeaklyNotTaken : StronglyNotTaken;
                    break;
            }
        }

        output << correct << "," << traces.size() << (size == 2048 ? ";\n" : "; ");
        std::cout << correct << "," << traces.size() << (size == 2048 ? ";\n" : "; ");
    }
}

void gsharePredictor(const std::vector<std::string>& traces, std::ofstream& output) {
    std::array<TwoBitSaturatingHistory, 2048> table{};
    for (auto& t : table) t = StronglyTaken;

    for (unsigned short mask = 0x7; mask <= 0x7FF; mask = (mask << 1) | 1) {
        std::fill(table.begin(), table.end(), StronglyTaken);
        GlobalHistoryRegister ghr{0};
        long correct = 0;

        for (const auto& trace : traces) {
            long address = std::stoul(trace.substr(0, 10), nullptr, 16);
            size_t index = (address ^ (ghr.bits & mask)) % 2048;
            auto& entry = table[index];
            bool isTaken = (trace[OUTCOME_INDEX] == TAKEN_CHAR);

            switch (entry) {
                case StronglyTaken:
                    correct += isTaken;
                    entry = isTaken ? StronglyTaken : WeaklyTaken;
                    break;
                case WeaklyTaken:
                    correct += isTaken;
                    entry = isTaken ? StronglyTaken : WeaklyNotTaken;
                    break;
                case WeaklyNotTaken:
                    correct += !isTaken;
                    entry = isTaken ? WeaklyTaken : StronglyNotTaken;
                    break;
                case StronglyNotTaken:
                    correct += !isTaken;
                    entry = isTaken ? WeaklyNotTaken : StronglyNotTaken;
                    break;
            }
            ghr.bits = (ghr.bits << 1) | static_cast<unsigned>(isTaken);
        }

        output << correct << "," << traces.size() << (mask == 0x7FF ? ";\n" : "; ");
        std::cout << correct << "," << traces.size() << (mask == 0x7FF ? ";\n" : "; ");
    }
}

void tournamentPredictor(const std::vector<std::string>& traces, std::ofstream& output) {
    std::array<TwoBitSaturatingHistory, 2048> gTable{}, bTable{};
    std::array<PredictorSelector, 2048> selectorTable{};
    std::fill(gTable.begin(), gTable.end(), StronglyTaken);
    std::fill(bTable.begin(), bTable.end(), StronglyTaken);
    std::fill(selectorTable.begin(), selectorTable.end(), StronglyGShare);
    GlobalHistoryRegister ghr{0};

    long correct = 0;

    for (const auto& trace : traces) {
        long address = std::stoul(trace.substr(0, 10), nullptr, 16);
        bool isTaken = (trace[OUTCOME_INDEX] == TAKEN_CHAR);

        size_t gIndex = (address ^ ghr.bits) % 2048;
        size_t bIndex = address % 2048;

        auto& gEntry = gTable[gIndex];
        auto& bEntry = bTable[bIndex];
        auto& sel = selectorTable[bIndex];

        bool gPredict = gEntry < WeaklyNotTaken;
        bool bPredict = bEntry < WeaklyNotTaken;

        bool gCorrect = (gPredict == isTaken);
        bool bCorrect = (bPredict == isTaken);

        // Update global history
        ghr.bits = (ghr.bits << 1) | static_cast<unsigned>(isTaken);

        // Update tables
        auto updateSaturating = [&](auto& entry) {
            if (isTaken) {
                entry = (entry == StronglyTaken) ? StronglyTaken : static_cast<TwoBitSaturatingHistory>(entry - 1);
            } else {
                entry = (entry == StronglyNotTaken) ? StronglyNotTaken : static_cast<TwoBitSaturatingHistory>(entry + 1);
            }
        };

        updateSaturating(gEntry);
        updateSaturating(bEntry);

        // Update selector
        if (gCorrect != bCorrect) {
            if (gCorrect && sel > StronglyGShare) sel = static_cast<PredictorSelector>(sel - 1);
            else if (bCorrect && sel < StronglyBimodal) sel = static_cast<PredictorSelector>(sel + 1);
        }

        // Final prediction
        if ((sel <= WeaklyGShare && gCorrect) || (sel >= WeaklyBimodal && bCorrect)) correct++;
    }

    output << correct << "," << traces.size() << ";\n";
    std::cout << correct << "," << traces.size() << ";\n";
}

int main(int argc, char* argv[]) {
    std::clock_t start = std::clock();

    if (argc < 2) {
        std::cerr << "Usage: ./predictor <trace_file>\n";
        return 1;
    }

    std::ifstream inputFile(argv[1]);
    std::ofstream outputFile("output.txt", std::ios::app);
    std::vector<std::string> traces;
    std::string line;

    while (std::getline(inputFile, line)) {
        traces.push_back(line);
    }

    alwaysTakenPredictor(traces, outputFile);
    neverTakenPredictor(traces, outputFile);
    singleBitBimodalPredictor(traces, outputFile);
    twoBitBimodalPredictor(traces, outputFile);
    gsharePredictor(traces, outputFile);
    tournamentPredictor(traces, outputFile);

    std::cout << "Time: " << std::fixed << std::setprecision(2)
              << (std::clock() - start) / (double)(CLOCKS_PER_SEC / 1000) << " ms\n";

    return 0;
}
