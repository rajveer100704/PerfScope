#pragma once
#include <queue>
#include <unordered_map>
#include <cstdint>
#include "config.hpp"

struct MemRequest {
    uint64_t instrID;
    uint64_t addr;
    int remainingCycles;
};

class MemorySystem {
public:
    MemorySystem(const SimConfig& cfg) : config(cfg) {}

    bool canAccept() const {
        return (int)queue.size() < config.memQueueSize;
    }

    void pushRequest(uint64_t id, uint64_t addr, int latency) {
        queue.push({id, addr, latency});
    }

    void processCycle() {
        int processed = 0;
        int initialSize = (int)queue.size();

        // Bandwidth-aware FIFO processing: 
        // Process up to memPorts requests at the front of the queue
        for (int i = 0; i < initialSize && processed < config.memPorts; ++i) {
            MemRequest req = queue.front();
            queue.pop();

            req.remainingCycles--;

            if (req.remainingCycles <= 0) {
                completionMap[req.instrID] = true;
            } else {
                // If not finished, push back to keep the order for the next cycle
                queue.push(req);
            }
            processed++;
        }
    }

    // Non-consuming check
    bool isDone(uint64_t id) const {
        return completionMap.count(id);
    }

    // Consuming check
    bool isCompleted(uint64_t id) {
        if (completionMap.count(id)) {
            completionMap.erase(id); // Consumer consumes the completion signal
            return true;
        }
        return false;
    }

    bool isBusy() const {
        return !queue.empty();
    }

    int getQueueSize() const {
        return (int)queue.size();
    }

private:
    const SimConfig& config;
    std::queue<MemRequest> queue;
    std::unordered_map<uint64_t, bool> completionMap; // Scoreboard
};
