#include "instruction.hpp"
#include <sstream>
#include <iomanip>
#include <iostream>

// Helper to parse "R12" into 12
int parseReg(const std::string& s) {
    if (s.empty() || s[0] != 'R') return -1;
    try {
        return std::stoi(s.substr(1));
    } catch (...) {
        return -1;
    }
}

Instruction parseInstruction(const std::string& line) {
    static uint64_t nextID = 0;
    std::istringstream iss(line);
    std::string op;
    iss >> op;

    Instruction instr;

    if (op == "LOAD") {
        std::string r; 
        uint64_t addr;
        iss >> r >> std::hex >> addr;
        instr = {InstrType::LOAD, addr, parseReg(r), {}};
    } else if (op == "STORE") {
        std::string r;
        uint64_t addr;
        iss >> r >> std::hex >> addr;
        instr = {InstrType::STORE, addr, -1, {parseReg(r)}};
    } else if (op == "ADD" || op == "SUB" || op == "MUL" || op == "DIV") {
        std::string rd, rs1, rs2;
        iss >> rd >> rs1 >> rs2;
        InstrType t = (op == "ADD" ? InstrType::ADD : 
                      (op == "SUB" ? InstrType::SUB :
                      (op == "MUL" ? InstrType::MUL : InstrType::DIV)));
        instr = {t, 0, parseReg(rd), {parseReg(rs1), parseReg(rs2)}};
    } else if (op == "BRANCH") {
        uint64_t pc;
        std::string r, takenStr;
        iss >> std::hex >> pc >> r >> takenStr;
        bool taken = (takenStr == "T");
        instr = {InstrType::BRANCH, 0, -1, {parseReg(r)}, taken, pc};
    } else {
        instr = {InstrType::NOP, 0};
    }

    instr.id = nextID++;
    return instr;
}
