#include "memory.hpp"
#include <iostream>
#include <iomanip>
#include <algorithm>

int Memory::access(uint64_t addr, int coreNodeId, uint64_t cycle) {
    uint64_t blockAddr = addr / 64;
    
    // Track overall frequency
    accessMap[blockAddr]++;

    // Handle cycle detection for contention
    if (cycle != currentCycle) {
        cycleAccesses.clear();
        currentCycle = cycle;
    }
    
    int currentContention = cycleAccesses[blockAddr]++;
    int baseLatency = numa.getLatency(coreNodeId, addr);
    
    // Add penalty for contention (simultaneous access to same block)
    int totalLatency = baseLatency + (currentContention * contentionPenalty);
    
    return totalLatency;
}

void Memory::visualizeContention() const {
    std::cout << "\n[Memory Block Contention Heatmap]\n";
    if (accessMap.empty()) return;

    // Sort by address for readability
    std::vector<std::pair<uint64_t, int>> sorted;
    for (auto const& [addr, freq] : accessMap) sorted.push_back({addr, freq});
    std::sort(sorted.begin(), sorted.end());

    for (const auto& pair : sorted) {
        if (pair.second < 2) continue; // Only show actually contended or frequently accessed blocks
        std::cout << "Block 0x" << std::hex << std::setw(8) << std::setfill('0') << pair.first << ": ";
        int bars = std::min(pair.second, 30);
        for (int b = 0; b < bars; ++b) std::cout << "*";
        std::cout << " (" << std::dec << pair.second << ")\n";
    }
}
