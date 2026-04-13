#pragma once
#include <unordered_map>
#include <cstdint>
#include <vector>
#include <mutex>
#include "numa.hpp"

class Memory {
private:
    NUMASystem numa;
    // Track access count per block (64 byte) for contention heatmap
    std::unordered_map<uint64_t, int> accessMap;
    // Current cycle's access counts for contention penalty
    std::unordered_map<uint64_t, int> cycleAccesses;
    uint64_t currentCycle = 0;
    int contentionPenalty;

public:
    Memory(int numNodes = 2, int localLat = 100, int remoteP = 50, int contP = 20)
        : numa(numNodes, localLat, remoteP), contentionPenalty(contP) {}

    // coreNodeId: where the request is coming from
    // returns total latency
    int access(uint64_t addr, int coreNodeId, uint64_t cycle);

    void visualizeContention() const;
    void printNUMAInfo() const { numa.printStats(); }
};
