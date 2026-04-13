#include "cache.hpp"
#include <iostream>
#include <algorithm>

Cache::Cache(const std::string& name, int size, int blockSize, int associativity)
    : name(name), size(size), blockSize(blockSize), associativity(associativity) {
    numSets = size / (blockSize * associativity);
    sets.resize(numSets, std::vector<CacheLine>(associativity));
    accessFrequency.resize(numSets, 0);
}

uint64_t Cache::getTag(uint64_t addr) const {
    return addr / (numSets * blockSize);
}

int Cache::getSetIndex(uint64_t addr) const {
    return (addr / blockSize) % numSets;
}

bool Cache::access(uint64_t addr, bool isWrite, uint64_t& outEvictedAddr, bool& outWasDirty) {
    int setIdx = getSetIndex(addr);
    uint64_t tag = getTag(addr);
    accessFrequency[setIdx]++;
    
    auto& set = sets[setIdx];
    outWasDirty = false;

    // 1. Check for Hit
    for (int i = 0; i < associativity; ++i) {
        if (set[i].valid && set[i].tag == tag) {
            hits++;
            // Update LRU: This one is 0, others increment
            for (int j = 0; j < associativity; ++j) {
                if (set[j].valid) set[j].lru++;
            }
            set[i].lru = 0;
            if (isWrite) set[i].dirty = true;
            return true;
        }
    }

    // 2. Miss
    misses++;
    
    // Find victim using LRU
    int victimIdx = 0;
    int maxLRU = -1;
    bool foundEmpty = false;

    for (int i = 0; i < associativity; ++i) {
        if (!set[i].valid) {
            victimIdx = i;
            foundEmpty = true;
            break;
        }
        if (set[i].lru > maxLRU) {
            maxLRU = set[i].lru;
            victimIdx = i;
        }
    }

    // If evicted line is dirty, prepare for writeback
    if (!foundEmpty && set[victimIdx].dirty) {
        outWasDirty = true;
        // Reconstruct evicted address
        outEvictedAddr = (set[victimIdx].tag * numSets + setIdx) * blockSize;
        writebacks++;
    }

    // Replace
    set[victimIdx].tag = tag;
    set[victimIdx].valid = true;
    set[victimIdx].dirty = isWrite; // If write miss, fetched then written -> dirty
    set[victimIdx].lru = 0;
    // Update others
    for (int i = 0; i < associativity; ++i) {
        if (i != victimIdx && set[i].valid) set[i].lru++;
    }

    return false;
}

void Cache::updateMESI(uint64_t addr, MESIState newState) {
    int setIdx = getSetIndex(addr);
    uint64_t tag = getTag(addr);
    auto& set = sets[setIdx];
    for (auto& line : set) {
        if (line.valid && line.tag == tag) {
            line.mesi = newState;
            if (newState == MESIState::INVALID) {
                line.valid = false;
                line.dirty = false;
            }
            break;
        }
    }
}

MESIState Cache::getMESI(uint64_t addr) const {
    int setIdx = getSetIndex(addr);
    uint64_t tag = getTag(addr);
    const auto& set = sets[setIdx];
    for (const auto& line : set) {
        if (line.valid && line.tag == tag) return line.mesi;
    }
    return MESIState::INVALID;
}

void Cache::visualizeHeatmap() const {
    std::cout << "\n[" << name << " Cache Heatmap]\n";
    int maxFreq = 0;
    for (int f : accessFrequency) if (f > maxFreq) maxFreq = f;
    if (maxFreq == 0) maxFreq = 1;

    for (int i = 0; i < numSets; ++i) {
        if (accessFrequency[i] == 0) continue;
        std::cout << "Set " << i << ": ";
        int bars = (accessFrequency[i] * 40) / maxFreq;
        for (int b = 0; b < bars; ++b) std::cout << "#";
        std::cout << " (" << accessFrequency[i] << ")\n";
    }
}
