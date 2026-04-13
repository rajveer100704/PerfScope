#pragma once
#include <cstdint>
#include <string>
#include <vector>

enum class InstrType {
    LOAD,
    STORE,
    ADD,
    SUB,
    MUL,
    DIV,
    BRANCH,
    NOP
};

struct Instruction {
    uint64_t id;           // Monotonic ID
    InstrType type;
    uint64_t address;
    int dest;              // Destination register (-1 if none)
    std::vector<int> src;  // Source register IDs
    bool branchTaken;      // For branch simulation
    uint64_t pc;           // For prediction logic

    Instruction(InstrType t = InstrType::NOP, uint64_t addr = 0, int d = -1, std::vector<int> s = {}, bool taken = false, uint64_t p = 0)
        : id(0), type(t), address(addr), dest(d), src(s), branchTaken(taken), pc(p) {}
};

// Parser
Instruction parseInstruction(const std::string& line);
