#pragma once
#include <vector>
#include <cstdint>

enum class MESIState {
    MODIFIED,
    EXCLUSIVE,
    SHARED,
    INVALID
};

struct MESILine {
    uint64_t tag = 0;
    MESIState state = MESIState::INVALID;
};

// Simplified coherence controller
class CoherenceManager {
public:
    // Handle read-miss behavior across all caches
    static void onReadMiss(uint64_t tag, int sourceId, std::vector<MESILine*>& lines) {
        bool foundElsewhere = false;
        for (size_t i = 0; i < lines.size(); ++i) {
            if (i == (size_t)sourceId) continue;
            if (lines[i]->tag == tag && lines[i]->state != MESIState::INVALID) {
                foundElsewhere = true;
                if (lines[i]->state == MESIState::MODIFIED) {
                    lines[i]->state = MESIState::SHARED; // Flush to mem then share
                } else if (lines[i]->state == MESIState::EXCLUSIVE) {
                    lines[i]->state = MESIState::SHARED;
                }
            }
        }
        // Source state set by cache caller (Shared if found elsewhere, else Exclusive)
    }

    // Handle write-hit/miss across all caches
    static void onWrite(uint64_t tag, int sourceId, std::vector<MESILine*>& lines) {
        for (size_t i = 0; i < lines.size(); ++i) {
            if (i == (size_t)sourceId) continue;
            if (lines[i]->tag == tag) {
                lines[i]->state = MESIState::INVALID; // Invalidate others
            }
        }
    }
};
