#pragma once
#include <cstdint>

struct SimConfig {
    // Latencies
    int aluLatency = 1;
    int mulLatency = 3;
    int divLatency = 10;
    int memLatency = 100; // Shared/System wide default
    int branchPenalty = 3;

    // Architectural Resources
    int aluUnits = 1;
    int memPorts = 1;
    int memQueueSize = 16;
    int numaNodes = 1;

    // Controls
    int seed = 42;
    bool debugMode = true;
    bool enableForwarding = true;
    uint64_t maxCycles = 1000000;

    // Default Constructor
    SimConfig() = default;
};
