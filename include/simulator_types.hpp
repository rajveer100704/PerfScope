#pragma once
#include <string>
#include <vector>
#include <cstdint>

struct SimResult {
    double ipc = 0.0;
    double backendOccupancy = 0.0;
    double backendThroughput = 0.0;
    
    // Cycle Attribution (Percents)
    double rawStallPct = 0.0;
    double branchStallPct = 0.0;
    double memWaitPct = 0.0;
    double structuralStallPct = 0.0;
    double idlePct = 0.0;
    
    // Resource Utilization
    double aluUtil = 0.0;
    double memPortUtil = 0.0;

    std::vector<std::string> diagnostics;
    uint64_t totalInstructions = 0;
    uint64_t totalCycles = 0;
};
