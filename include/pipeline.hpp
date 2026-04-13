#pragma once
#include "instruction.hpp"
#include "config.hpp"
#include "status.hpp"
#include <functional>
#include <vector>
#include <string>
#include <iostream>
#include <set>
#include <cassert>

namespace Color {
    const std::string RESET = "\033[0m";
    const std::string RED = "\033[31m";
    const std::string GREEN = "\033[32m";
    const std::string YELLOW = "\033[33m";
    const std::string BLUE = "\033[34m";
    const std::string MAGENTA = "\033[35m";
    const std::string CYAN = "\033[36m";
    const std::string BRIGHT_GREEN = "\033[92m";
}

struct Stage {
    Instruction instr;
    bool valid = false;
    int remainingCycles = 0;
    int result = 0;          
    bool resultReady = false;
    bool justEntered = false; // Fix for latency bug
    bool requestIssued = false;
    bool waitingForMemory = false;

    void clear() {
        valid = false;
        remainingCycles = 0;
        resultReady = false;
        justEntered = false;
    }
};

struct ForwardEvent {
    std::string fromStage;
    std::string toStage;
    int reg;
};

#include "resource_manager.hpp"
#include "memory_system.hpp"

class Pipeline {
public:
    Stage IF, ID, EX, MEM, WB;
    uint64_t cycles = 0;
    uint64_t retired = 0;
    
    // Priority Stall Counters
    uint64_t rawStallCycles = 0;
    uint64_t memWaitCycles = 0; // Inherent latency wait (non-structural)
    uint64_t aluStallCycles = 0;
    uint64_t branchStallCycles = 0;
    uint64_t memQueueStallCycles = 0;
    uint64_t memPortStallCycles = 0;
    uint64_t idleCycles = 0; // No useful work being processed

    // Utilization & Occupancy Tracking
    uint64_t aluUsageCycles = 0;
    uint64_t memPortUsageCycles = 0;
    uint64_t exOccupancyCycles = 0; // Total cycles EX stage is occupied (valid)
    uint64_t totalQueueDepth = 0;
    uint64_t maxQueueDepth = 0;

    // Utilization Tracking
    uint64_t activeSlots = 0; // Strict: valid and not stalled
    
    int forwardedCount = 0;
    int branchRecoveryCycles = 0; // Penalty counter
    std::vector<ForwardEvent> forwardEvents;

    Pipeline(const SimConfig& cfg);

    SimStatus tick(const SimConfig& config, std::function<int(uint64_t)> getLat = nullptr);
    void flush();
    SimStatus validatePipeline(const SimConfig& config) const;
    bool hasRAWHazard(const Instruction& curr, const Stage& stage) const;
    bool canForward(const Instruction& curr, const Stage& stage) const;
    bool isBusy() const;

    void printState() const;
    void printForwarding() const;
    void clearForwardEvents() { forwardEvents.clear(); }

    uint64_t getTotalStalls() const {
        // Excludes memWaitCycles (latency wait) to separate Structural/Logic from Architecture
        return rawStallCycles + aluStallCycles + branchStallCycles + memQueueStallCycles + memPortStallCycles;
    }

private:
    const SimConfig& config;
    ResourceManager resources;
    MemorySystem memory;

    std::string formatStage(const Stage& s, const std::string& color) const;
    std::string instrName(const Instruction& instr) const;
    bool isALUOp(const Instruction& instr) const;
    bool isMemoryOp(const Instruction& instr) const;
};
