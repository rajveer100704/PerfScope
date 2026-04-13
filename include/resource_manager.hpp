#pragma once
#include "config.hpp"

class ResourceManager {
public:
    ResourceManager(const SimConfig& cfg) : config(cfg) {}

    void resetCycle() {
        aluUsed = 0;
        memPortsUsed = 0;
    }

    bool requestALU() {
        if (aluUsed < config.aluUnits) {
            aluUsed++;
            return true;
        }
        return false;
    }

    bool requestMemPort() {
        if (memPortsUsed < config.memPorts) {
            memPortsUsed++;
            return true;
        }
        return false;
    }

    // For metrics
    int getAluUsed() const { return aluUsed; }
    int getMemPortsUsed() const { return memPortsUsed; }

private:
    const SimConfig& config;
    int aluUsed = 0;
    int memPortsUsed = 0;
};
