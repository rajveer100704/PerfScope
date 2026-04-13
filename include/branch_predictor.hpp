#pragma once
#include <unordered_map>
#include <vector>
#include <cstdint>
#include <iostream>
#include <string>

enum class PredictorType {
    STATIC,
    GSHARE
};

class BranchPredictor {
private:
    PredictorType type;
    std::vector<int> table; // 2-bit counters (00: ST, 01: WT, 10: WNT, 11: SNT - or 0-3 saturating)
    // Using 0: SNT, 1: WNT, 2: WT, 3: ST logic
    uint32_t globalHistory = 0;
    int historyBits;

public:
    int mispredictions = 0;
    int total = 0;

    BranchPredictor(PredictorType t = PredictorType::STATIC, int bits = 8)
        : type(t), historyBits(bits) {
        if (type == PredictorType::GSHARE) {
            table.resize(1ULL << bits, 1); // weekly not taken
        }
    }

    bool predict(uint64_t pc) {
        if (type == PredictorType::STATIC) {
            return true; // Always taken
        }

        uint32_t index = (pc ^ globalHistory) & ((1U << historyBits) - 1);
        return table[index] >= 2;
    }

    void update(uint64_t pc, bool taken) {
        total++;
        bool pred = predict(pc);
        if (pred != taken) mispredictions++;

        if (type == PredictorType::GSHARE) {
            uint32_t index = (pc ^ globalHistory) & ((1U << historyBits) - 1);
            int& state = table[index];
            if (taken && state < 3) state++;
            else if (!taken && state > 0) state--;

            globalHistory = ((globalHistory << 1) | (taken ? 1U : 0U)) & ((1U << historyBits) - 1);
        }
    }

    void printStats(const std::string& name) const {
        if (total == 0) return;
        float rate = (float)mispredictions / total * 100;
        std::cout << "[" << name << " Predictor] "
                  << "Mispredictions: " << mispredictions 
                  << " / " << total 
                  << " (" << rate << "%)" << std::endl;
    }

    static void evaluate(uint64_t pc, bool taken, BranchPredictor& staticP, BranchPredictor& gshareP) {
        staticP.update(pc, taken);
        gshareP.update(pc, taken);
    }
};
