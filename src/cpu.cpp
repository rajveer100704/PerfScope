#include "cpu.hpp"
#include <iostream>
#include <iomanip>
#include <random>

CPU::CPU(int numCores, int l1Size, int l2Size, int numaNodes, SimConfig cfg)
    : l2("L2_Shared", l2Size, 64, 8), 
      memory(numaNodes, 100, 50, 20),
      coreNodeCount(numaNodes),
      config(cfg) {
    for (int i = 0; i < numCores; ++i) {
        cores.push_back(std::make_unique<Core>(i, i % numaNodes, l1Size, cfg));
    }
}

void CPU::handleCoherence(int sourceCoreId, uint64_t addr, bool isWrite) {
    // Functional MESI-style coherence model (Snooping)
    for (const auto& core : cores) {
        if (core->id != sourceCoreId) {
            core->l1.updateMESI(addr, MESIState::INVALID);
        }
    }
}

int CPU::accessMemory(Core& core, uint64_t addr, bool isWrite, uint64_t cycle) {
    uint64_t evictedAddr;
    bool wasDirty;
    int latency = 1; // L1 hit minimum

    if (core.l1.access(addr, isWrite, evictedAddr, wasDirty)) {
        // L1 Hit
    } else {
        // L1 Miss -> L2
        latency += 10;
        if (l2.access(addr, isWrite, evictedAddr, wasDirty)) {
            // L2 Hit
        } else {
            // L2 Miss -> Memory
            latency += memory.access(addr, core.nodeId, cycle);
        }
    }
    
    if (isWrite) {
        handleCoherence(core.id, addr, true);
    }

    return latency;
}

SimStatus CPU::run(const std::vector<std::vector<Instruction>>& threadPrograms) {
    int numThreads = static_cast<int>(threadPrograms.size());
    std::vector<int> pc(numThreads, 0);
    bool allDone = false;
    uint64_t globalCycle = 0;

    while (!allDone) {
        allDone = true;
        globalCycle++;

        if (globalCycle > config.maxCycles) {
            std::cerr << "Simulation aborted: Infinite loop detected (reached " << config.maxCycles << " cycles).\n";
            return SimStatus::INFINITE_LOOP;
        }

        for (int t = 0; t < numThreads; ++t) {
            Core& core = *cores[t % cores.size()];
            Pipeline& pipe = core.pipeline;
            pipe.clearForwardEvents();

            // FETCH
            if (!pipe.IF.valid && pc[t] < threadPrograms[t].size()) {
                pipe.IF.instr = threadPrograms[t][pc[t]++];
                pipe.IF.valid = true;
                pipe.IF.justEntered = true; // Crucial for snapshot timing
                core.instructionsExecuted++;
            }

            // BRANCH Prediction handling (EX stage)
            if (pipe.EX.valid && pipe.EX.instr.type == InstrType::BRANCH && pipe.EX.justEntered) {
                Instruction& instr = pipe.EX.instr;
                bool pred = core.gsharePred.predict(instr.pc);
                core.staticPred.update(instr.pc, instr.branchTaken);
                core.gsharePred.update(instr.pc, instr.branchTaken);

                if (pred != instr.branchTaken) {
                    pipe.flush();
                }
                pipe.EX.justEntered = false;
            }

            // TICK with memory latency callback
            auto getLat = [&](uint64_t addr) {
                bool isStore = pipe.EX.instr.type == InstrType::STORE;
                return accessMemory(core, addr, isStore, globalCycle);
            };

            SimStatus status = pipe.tick(config, getLat);
            if (status != SimStatus::OK) return status;
            
            if (pipe.isBusy() || pc[t] < threadPrograms[t].size()) {
                allDone = false;
            }
        }
    }
    return SimStatus::OK;
}

void CPU::printFinalStats() const {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "         ELITE ARCHITECTURAL ANALYSIS REPORT\n";
    std::cout << std::string(60, '=') << "\n";

    for (const auto& core : cores) {
        const auto& p = core->pipeline;
        uint64_t totalCycles = p.cycles > 0 ? p.cycles : 1;
        float ipc = (float)p.retired / totalCycles;
        float efficiency = (ipc / 1.0f) * 100.0f; // Max IPC = 1.0 (Single-Issue)
        
        std::cout << "\n>> Core " << core->id << " Execution Profile:\n";
        std::cout << "   - Retired Instructions: " << p.retired << "\n";
        std::cout << "   - Total Cycles:        " << p.cycles << "\n";
        std::cout << "   - IPC / Efficiency:    " << std::fixed << std::setprecision(3) << ipc 
                  << " (" << std::fixed << std::setprecision(1) << efficiency << "% of Max)\n";
        std::cout << "     [Overall pipeline throughput, including Idle/Frontend bubbles]\n";

        std::cout << "\nBackend Throughput Metrics:\n";
        double exOccupancy = (double)p.exOccupancyCycles / totalCycles * 100.0;
        double throughput = (p.exOccupancyCycles > 0) ? (double)p.retired / p.exOccupancyCycles : 0;
        std::cout << "     * Backend Occupancy:  " << std::fixed << std::setprecision(1) << exOccupancy << "%\n";
        std::cout << "     * Backend Throughput: " << std::fixed << std::setprecision(3) << throughput << " IPC (EX-active cycles)\n";
        std::cout << "     [Progress rate while Execution units are valid (typically > Overall IPC)]\n";

        std::cout << "\nCycle Attribution (% of Cycles):\n";
        auto printLoss = [&](const std::string& name, uint64_t val) {
            double pct = (double)val / totalCycles * 100.0;
            std::cout << "     - " << std::left << std::setw(15) << name << ": " << std::fixed << std::setprecision(1) << pct << "%\n";
        };
        printLoss("RAW Hazard", p.rawStallCycles);
        printLoss("Branch Stalls", p.branchStallCycles);
        printLoss("Memory Wait", p.memWaitCycles);
        printLoss("Structural", p.aluStallCycles + p.memPortStallCycles + p.memQueueStallCycles);
        printLoss("Idle/Frontend", p.idleCycles);

        std::cout << "\nFunctional Unit Utilization:\n";
        double aluUtil = (double)p.aluUsageCycles / (totalCycles * config.aluUnits) * 100.0;
        double memUtil = (double)p.memPortUsageCycles / (totalCycles * config.memPorts) * 100.0;
        std::cout << "     * ALU Busy:     " << std::fixed << std::setprecision(1) << aluUtil << "%\n";
        std::cout << "     * Mem Port:     " << std::fixed << std::setprecision(1) << memUtil << "%\n";
        
        std::cout << "\nArchitectural Diagnostic & Scaling Insights:\n";
        if (p.aluStallCycles > p.cycles * 0.1) 
            std::cout << "     -> [DIAGNOSTIC] ALU unit contention detected. Scaling --aluUnits would boost IPC.\n";
        if (p.memWaitCycles > p.cycles * 0.3)
            std::cout << "     -> [DIAGNOSTIC] Dominant Memory Wait. Optimize locality or NUMA mapping.\n";
        if (p.rawStallCycles > p.cycles * 0.2)
            std::cout << "     -> [DIAGNOSTIC] High RAW Stall rate. Pipeline is waiting on dependencies.\n";

        std::cout << "========================================\n\n";
        core->gsharePred.printStats("Branch Predictor");
    }
}
