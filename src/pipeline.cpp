#include "pipeline.hpp"
#include "status.hpp"
#include <functional>
#include <iostream>
#include <iomanip>
#include <set>
#include <algorithm>

Pipeline::Pipeline(const SimConfig& cfg) 
    : config(cfg), resources(cfg), memory(cfg) {
    IF.clear(); ID.clear(); EX.clear(); MEM.clear(); WB.clear();
}

bool Pipeline::hasRAWHazard(const Instruction& curr, const Stage& stage) const {
    if (!stage.valid || stage.instr.dest == -1) return false;
    for (int srcReg : curr.src) {
        if (srcReg != -1 && srcReg == stage.instr.dest) return true;
    }
    return false;
}

bool Pipeline::canForward(const Instruction& curr, const Stage& stage) const {
    // Structural change: for memory ops, we only forward if it's already completed.
    // For ALU ops, results are ready once remainingCycles == 0.
    if (!stage.valid || !stage.resultReady || stage.instr.dest == -1) return false;
    for (int srcReg : curr.src) {
        if (srcReg != -1 && srcReg == stage.instr.dest) return true;
    }
    return false;
}

bool Pipeline::isALUOp(const Instruction& instr) const {
    return instr.type == InstrType::ADD || instr.type == InstrType::SUB || 
           instr.type == InstrType::MUL || instr.type == InstrType::DIV;
}

bool Pipeline::isMemoryOp(const Instruction& instr) const {
    return instr.type == InstrType::LOAD || instr.type == InstrType::STORE;
}

SimStatus Pipeline::validatePipeline(const SimConfig& config) const {
    std::set<uint64_t> ids;
    auto check = [&](const Stage& s) {
        if (s.valid) {
            if (ids.count(s.instr.id)) return false;
            ids.insert(s.instr.id);
        }
        return true;
    };

    if (!check(IF) || !check(ID) || !check(EX) || !check(MEM) || !check(WB)) {
        if (config.debugMode) {
            std::cerr << "Pipeline Invariant Failed: Duplicate instruction ID detected.\n";
            assert(false);
        }
        return SimStatus::INVARIANT_FAILED;
    }
    return SimStatus::OK;
}

SimStatus Pipeline::tick(const SimConfig& cfg, std::function<int(uint64_t)> getLat) {
    if (cfg.debugMode) {
        SimStatus status = validatePipeline(cfg);
        if (status != SimStatus::OK) return status;
    }
    
    cycles++;

    // 1. Process Hardware Resources & Bandwidth
    resources.resetCycle();
    memory.processCycle();

    // 2. Advanced Metrics
    uint64_t qLen = memory.getQueueSize();
    totalQueueDepth += qLen;
    maxQueueDepth = std::max(maxQueueDepth, qLen);

    // 3. Hazard and Stall Detection
    bool hasBranchStall = (branchRecoveryCycles > 0);
    bool hasRawStall = false;
    bool hasMemQueueStall = (EX.valid && EX.remainingCycles == 0 && isMemoryOp(EX.instr) && !memory.canAccept());
    bool hasMemPortStall = (EX.valid && EX.remainingCycles == 0 && isMemoryOp(EX.instr) && memory.canAccept() && resources.getMemPortsUsed() >= cfg.memPorts);
    bool hasAluStall = (EX.valid && EX.remainingCycles == 0 && isALUOp(EX.instr) && resources.getAluUsed() >= cfg.aluUnits);
    
    // Memory Scoreboard stall (MEM stage waiting for its specific ID)
    bool hasAsyncMemStall = (MEM.valid && MEM.waitingForMemory && !memory.isDone(MEM.instr.id));

    if (ID.valid && !hasBranchStall) {
        bool hazardEX = hasRAWHazard(ID.instr, EX);
        bool hazardMEM = hasRAWHazard(ID.instr, MEM);
        bool hazardWB = hasRAWHazard(ID.instr, WB);
        
        bool fwdEX = cfg.enableForwarding && canForward(ID.instr, EX);
        bool fwdMEM = cfg.enableForwarding && canForward(ID.instr, MEM);
        bool fwdWB = cfg.enableForwarding && canForward(ID.instr, WB);
        
        bool loadUse = EX.valid && EX.instr.type == InstrType::LOAD && hasRAWHazard(ID.instr, EX);

        if (loadUse || (hazardEX && !fwdEX) || (hazardMEM && !fwdMEM) || (hazardWB && !fwdWB)) {
            hasRawStall = true;
        }
    }

    // Assign Stall cycle (Strict Priority: Branch > RAW > MemQueue > MemPort > ALU > AsyncMem)
    if (hasBranchStall) { 
        branchStallCycles++;
        branchRecoveryCycles--;
    } else if (hasRawStall) {
        rawStallCycles++;
    } else if (hasMemQueueStall) {
        memQueueStallCycles++;
    } else if (hasMemPortStall) {
        memPortStallCycles++;
    } else if (hasAluStall) {
        aluStallCycles++;
    } else if (hasAsyncMemStall) {
        memWaitCycles++;
    }
    
    // 4. Idle Detection (Strict)
    // A cycle is idle if no stage is performing useful productive work
    bool anyProductive = (IF.valid && !hasBranchStall && !hasRawStall) || 
                          (ID.valid && !hasRawStall) || 
                          (EX.valid && EX.remainingCycles > 0) || 
                          (MEM.valid && !MEM.waitingForMemory) || 
                          (WB.valid);
    if (!anyProductive) idleCycles++;

    // 4. Resource Utilization Update (Per-Cycle)
    if (EX.valid) exOccupancyCycles++;
    if (EX.valid && isALUOp(EX.instr)) aluUsageCycles++;
    if (EX.valid && isMemoryOp(EX.instr) && EX.requestIssued) memPortUsageCycles++;

    // 5. Stage Movement Snapshots (Enforce 1 Cycle Per Stage in Backwards Loop)
    bool wb_was_full = WB.valid;
    bool mem_was_full = MEM.valid;
    bool ex_was_full = EX.valid;
    bool id_was_full = ID.valid;
    bool if_was_full = IF.valid;

    // RETIRE WB
    if (WB.valid) {
        retired++;
        WB.clear();
    }

    // MEM -> WB (Decoupled/Scoreboarding)
    if (MEM.valid) {
        if (MEM.waitingForMemory) {
            if (memory.isDone(MEM.instr.id)) {
                MEM.waitingForMemory = false;
                MEM.resultReady = true;
            }
        }

        if (!MEM.waitingForMemory && !wb_was_full) {
            memory.isCompleted(MEM.instr.id); // Consume the completion
            WB = MEM;
            WB.justEntered = true;
            MEM.clear();
        }
    }

    // EX -> MEM (Structural Hazards)
    if (EX.valid) {
        if (EX.remainingCycles > 0) {
            EX.remainingCycles--;
        } else if (!mem_was_full) {
            bool canMove = false;
            if (isMemoryOp(EX.instr)) {
                if (memory.canAccept() && resources.requestMemPort()) {
                    int lat = getLat ? getLat(EX.instr.address) : config.memLatency;
                    memory.pushRequest(EX.instr.id, EX.instr.address, lat);
                    EX.requestIssued = true;
                    EX.waitingForMemory = true;
                    canMove = true;
                }
            } else if (isALUOp(EX.instr)) {
                if (resources.requestALU()) {
                    canMove = true;
                }
            } else {
                canMove = true; // BRANCH, NOP etc
            }

            if (canMove) {
                MEM = EX;
                MEM.justEntered = true;
                EX.clear();
            }
        }
    }

    // ID -> EX
    if (ID.valid && !hasBranchStall && !hasRawStall && !ex_was_full) {
        if (cfg.enableForwarding) {
            if (canForward(ID.instr, EX)) { forwardEvents.push_back({"EX", "ID", (int)EX.instr.dest}); forwardedCount++; }
            if (canForward(ID.instr, MEM)) { forwardEvents.push_back({"MEM", "ID", (int)MEM.instr.dest}); forwardedCount++; }
            if (canForward(ID.instr, WB)) { forwardEvents.push_back({"WB", "ID", (int)WB.instr.dest}); forwardedCount++; }
        }

        EX = ID;
        EX.justEntered = true;
        ID.clear();

        if (EX.instr.type == InstrType::MUL) EX.remainingCycles = cfg.mulLatency - 1;
        else if (EX.instr.type == InstrType::DIV) EX.remainingCycles = cfg.divLatency - 1;
        else EX.remainingCycles = cfg.aluLatency - 1;

        if (EX.remainingCycles == 0 && !isMemoryOp(EX.instr) && EX.instr.type != InstrType::BRANCH) {
            EX.resultReady = true;
        } else {
            EX.resultReady = false;
        }
    }

    // IF -> ID
    if (IF.valid && !hasBranchStall && !hasRawStall && !id_was_full) {
        ID = IF;
        ID.justEntered = true;
        IF.clear();
    }

    return SimStatus::OK;
}

void Pipeline::flush() {
    IF.clear(); ID.clear(); EX.clear(); MEM.clear();
    branchRecoveryCycles = 3; 
}

bool Pipeline::isBusy() const {
    return IF.valid || ID.valid || EX.valid || MEM.valid || WB.valid || branchRecoveryCycles > 0 || memory.isBusy();
}

std::string Pipeline::instrName(const Instruction& instr) const {
    switch (instr.type) {
        case InstrType::LOAD: return "LD";
        case InstrType::STORE: return "ST";
        case InstrType::ADD: return "ADD";
        case InstrType::SUB: return "SUB";
        case InstrType::MUL: return "MUL";
        case InstrType::DIV: return "DIV";
        case InstrType::BRANCH: return "BR";
        default: return "NOP";
    }
}

std::string Pipeline::formatStage(const Stage& s, const std::string& color) const {
    if (!s.valid) return ".";
    return color + instrName(s.instr) + Color::RESET;
}

void Pipeline::printState() const {
    std::cout << " Cycle " << std::setw(3) << cycles << " | ";
    std::cout << "IF:" << formatStage(IF, Color::BLUE) << " "
              << "ID:" << formatStage(ID, Color::CYAN) << " "
              << "EX:" << formatStage(EX, Color::YELLOW) << " "
              << "MEM:" << formatStage(MEM, Color::MAGENTA) << " "
              << "WB:" << formatStage(WB, Color::GREEN) << " | ";
}

void Pipeline::printForwarding() const {
    if (forwardEvents.empty()) return;
    std::cout << Color::BRIGHT_GREEN << "FWD: ";
    for (const auto& ev : forwardEvents) {
        std::cout << ev.fromStage << "->" << ev.toStage << "(R" << ev.reg << ") ";
    }
    std::cout << Color::RESET;
}
