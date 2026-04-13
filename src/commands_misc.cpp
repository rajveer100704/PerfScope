#include "commands.hpp"
#include "simulator.hpp"
#include "logger.hpp"
#include <iostream>
#include <iomanip>

void benchmarkCommand(const CLI& cli) {
    Logger::info("Running architectural benchmark suite...");
    
    std::vector<std::string> traces = { "compute_heavy.trace", "memory_heavy.trace" };
    
    if (cli.hasFlag("--json")) std::cout << "[\n";

    for (size_t i = 0; i < traces.size(); ++i) {
        SimConfig cfg;
        Simulator sim(cfg);
        SimResult res = sim.run(traces[i]);
        
        if (cli.hasFlag("--json")) {
            // Minimal JSON for benchmark list
            std::cout << "  {\"trace\": \"" << traces[i] << "\", \"ipc\": " << res.ipc << "}";
            if (i + 1 < traces.size()) std::cout << ",";
            std::cout << "\n";
        } else {
            std::cout << traces[i] << " -> IPC: " << std::fixed << std::setprecision(3) << res.ipc << "\n";
        }
    }
    
    if (cli.hasFlag("--json")) std::cout << "]\n";
}

void sweepCommand(const CLI& cli) {
    Logger::info("Parameter sweep not yet fully implemented in this version.");
}

void inspectCommand(const CLI& cli) {
    Logger::info("Detailed pipeline inspection requires --debug and a small trace.");
}
