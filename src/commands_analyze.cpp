#include "commands.hpp"
#include "simulator.hpp"
#include "logger.hpp"
#include <iostream>
#include <iomanip>
#include <thread>
#include <fstream>

void analyzeSystemCommand(const CLI& cli) {
    Logger::info("Analyzing local hardware baseline...");

    // Hardware detection
    unsigned int cores = std::thread::hardware_concurrency();
    Logger::info("System Detected: " + std::to_string(cores) + " logical processors.");

    // Synthetic Analysis
    // We run a 'balanced' trace as a baseline test for the current device capability
    std::string tracePath = "traces/sample.trace"; // Fallback
    
    // Check if we have pre-generated traces
    std::ifstream check("compute_heavy.trace");
    if (check.good()) tracePath = "compute_heavy.trace";

    Logger::info("Running synthetic diagnostic workload...");

    SimConfig cfg;
    cfg.numaNodes = (cores > 8) ? 2 : 1; // Basic heuristic
    
    Simulator sim(cfg);
    SimResult res = sim.run(tracePath);

    if (cli.hasFlag("--json")) {
        // Simple JSON with system info added
        std::cout << "{\n  \"system_info\": {\"cores\": " << cores << "},\n";
        std::cout << "  \"baseline_ipc\": " << res.ipc << "\n}\n";
    } else {
        std::cout << "\nSYSTEM ARCHITECTURAL BASELINE\n";
        std::cout << "----------------------------\n";
        std::cout << "Detected Cores: " << cores << "\n";
        std::cout << "Simulated IPC:  " << res.ipc << "\n";
        std::cout << "Frontend Idle:  " << std::fixed << std::setprecision(1) << res.idlePct * 100.0 << "%\n";
        std::cout << "\n[ADVISORY]\n";
        if (res.ipc < 0.15) {
            std::cout << "  -> System exhibits high latency under this workload. Check NUMA distances.\n";
        } else {
            std::cout << "  -> System balanced for architectural performance studies.\n";
        }
    }
}
