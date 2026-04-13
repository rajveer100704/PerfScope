#pragma once
#include "cache.hpp"
#include "memory.hpp"
#include "pipeline.hpp"
#include "instruction.hpp"
#include "branch_predictor.hpp"
#include "status.hpp"
#include "config.hpp"
#include "simulator_types.hpp"
#include <vector>
#include <memory>

struct Core {
    int id;
    int nodeId;
    Pipeline pipeline;
    BranchPredictor staticPred;
    BranchPredictor gsharePred;
    Cache l1;
    int instructionsExecuted = 0;

    Core(int id, int nodeId, int l1Size, const SimConfig& cfg)
        : id(id), nodeId(nodeId), 
          pipeline(cfg),
          staticPred(PredictorType::STATIC), 
          gsharePred(PredictorType::GSHARE),
          l1("Core" + std::to_string(id) + "_L1", l1Size, 64, 4) {}
};

class CPU {
private:
    std::vector<std::unique_ptr<Core>> cores;
    Cache l2;
    Memory memory;
    int coreNodeCount;
    SimConfig config;

    // Coherence handling
    void handleCoherence(int sourceCoreId, uint64_t addr, bool isWrite);

public:
    CPU(int numCores, int l1Size, int l2Size, int numaNodes, SimConfig config = SimConfig());

    SimStatus run(const std::vector<std::vector<Instruction>>& threadPrograms);
    
    // Memory hierarchy access
    int accessMemory(Core& core, uint64_t addr, bool isWrite, uint64_t cycle);

    void printFinalStats() const;
    std::vector<SimResult> getResults() const;
};
