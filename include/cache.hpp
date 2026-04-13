#pragma once
#include <vector>
#include <cstdint>
#include <string>
#include "mesi.hpp"

struct CacheLine {
    uint64_t tag = 0;
    bool valid = false;
    bool dirty = false;
    int lru = 0;
    MESIState mesi = MESIState::INVALID;
};

class Cache {
private:
    std::string name;
    int size;
    int blockSize;
    int associativity;
    int numSets;

    std::vector<std::vector<CacheLine>> sets;
    std::vector<int> accessFrequency;

    uint64_t getTag(uint64_t addr) const;
    int getSetIndex(uint64_t addr) const;

public:
    int hits = 0;
    int misses = 0;
    int writebacks = 0;

    Cache(const std::string& name, int size, int blockSize, int associativity);

    // Returns true on hit, false on miss
    // isWrite: for dirty bit management
    // outEvictedAddr: if a dirty line is evicted, returns its address for writeback
    bool access(uint64_t addr, bool isWrite, uint64_t& outEvictedAddr, bool& outWasDirty);

    void updateMESI(uint64_t addr, MESIState newState);
    MESIState getMESI(uint64_t addr) const;

    void visualizeHeatmap() const;
    
    // Stats
    int getHits() const { return hits; }
    int getMisses() const { return misses; }
    int getWritebacks() const { return writebacks; }
    std::string getName() const { return name; }
};
