#include "simulator.hpp"
#include "cpu.hpp"
#include "logger.hpp"
#include <fstream>
#include <iostream>

Simulator::Simulator(const SimConfig& cfg) : config(cfg) {}

SimResult Simulator::run(const std::string& tracePath) {
    Logger::debug("Loading trace: " + tracePath);
    
    std::ifstream file(tracePath);
    if (!file.is_open()) {
        Logger::error("Could not open trace file: " + tracePath);
        exit(1);
    }

    std::vector<Instruction> mainProgram;
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;
        mainProgram.push_back(parseInstruction(line));
    }

    if (mainProgram.empty()) {
        Logger::error("Trace file is empty or invalid: " + tracePath);
        exit(1);
    }

    // Single core simulation for now (as per common CLI use cases)
    // We can scale this based on SimConfig if added later
    std::vector<std::vector<Instruction>> threadPrograms(1);
    threadPrograms[0] = mainProgram;

    Logger::info("Starting simulation of " + std::to_string(mainProgram.size()) + " instructions...");
    
    // Core parameters (could be pulled from config later)
    int l1Size = 32 * 1024;
    int l2Size = 256 * 1024;
    int numaNodes = config.numaNodes;

    CPU cpu(1, l1Size, l2Size, numaNodes, config);
    SimStatus status = cpu.run(threadPrograms);

    if (status != SimStatus::OK) {
        Logger::error("Simulation failed with status code: " + std::to_string((int)status));
        exit(1);
    }

    // Extract results using the newly added getResults API
    std::vector<SimResult> results = cpu.getResults();
    
    if (results.empty()) {
        Logger::error("No results generated from simulation.");
        exit(1);
    }

    return results[0]; 
}
