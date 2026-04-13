#pragma once
#include <vector>
#include <cstdint>
#include <iostream>

struct NUMANode {
    int id;
    int baseLatency; // Local latency
};

class NUMASystem {
private:
    std::vector<NUMANode> nodes;
    int remotePenalty;

public:
    NUMASystem(int numNodes = 2, int localLat = 100, int remoteP = 50) 
        : remotePenalty(remoteP) {
        for (int i = 0; i < numNodes; ++i) {
            nodes.push_back({i, localLat});
        }
    }

    int getLatency(int coreNodeId, uint64_t addr) const {
        // Map address to home node (interleaved or block based)
        // Using block size of 4KB for page-level NUMA mapping
        int homeNode = static_cast<int>((addr / 4096) % nodes.size());
        
        if (coreNodeId == homeNode) {
            return nodes[coreNodeId].baseLatency;
        } else {
            return nodes[homeNode].baseLatency + remotePenalty;
        }
    }

    int getNumNodes() const { return static_cast<int>(nodes.size()); }

    void printStats() const {
        std::cout << "\n[NUMA Configuration]\n";
        for (const auto& node : nodes) {
            std::cout << "Node " << node.id << ": Base Latency " << node.baseLatency << " cycles\n";
        }
        std::cout << "Remote Access Penalty: " << remotePenalty << " cycles\n";
    }
};
