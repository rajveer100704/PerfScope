#include "cpu.hpp"
#include "simulator.hpp" // For SimResult

std::vector<SimResult> CPU::getResults() const {
    std::vector<SimResult> results;
    for (const auto& core : cores) {
        const auto& p = core->pipeline;
        SimResult res;
        res.totalInstructions = p.retired;
        res.totalCycles = p.cycles;
        
        uint64_t totalCycles = p.cycles > 0 ? p.cycles : 1;
        res.ipc = (double)p.retired / totalCycles;
        
        res.backendOccupancy = (double)p.exOccupancyCycles / totalCycles;
        res.backendThroughput = (p.exOccupancyCycles > 0) ? (double)p.retired / p.exOccupancyCycles : 0;
        
        res.rawStallPct = (double)p.rawStallCycles / totalCycles;
        res.branchStallPct = (double)p.branchStallCycles / totalCycles;
        res.memWaitPct = (double)p.memWaitCycles / totalCycles;
        res.structuralStallPct = (double)(p.aluStallCycles + p.memPortStallCycles + p.memQueueStallCycles) / totalCycles;
        res.idlePct = (double)p.idleCycles / totalCycles;
        
        res.aluUtil = (double)p.aluUsageCycles / (totalCycles * config.aluUnits);
        res.memPortUtil = (double)p.memPortUsageCycles / (totalCycles * config.memPorts);
        
        if (p.aluStallCycles > p.cycles * 0.1) 
            res.diagnostics.push_back("ALU unit contention detected. Scaling --aluUnits would boost IPC.");
        if (p.memWaitCycles > p.cycles * 0.3)
            res.diagnostics.push_back("Dominant Memory Wait. Optimize locality or NUMA mapping.");
        if (p.rawStallCycles > p.cycles * 0.2)
            res.diagnostics.push_back("High RAW Stall rate. Pipeline is waiting on dependencies.");
            
        results.push_back(res);
    }
    return results;
}
