#include "commands.hpp"
#include "simulator.hpp"
#include "logger.hpp"
#include <iostream>
#include <iomanip>

void printJSON(const SimResult& r) {
    std::cout << "{\n";
    std::cout << "  \"ipc\": " << std::fixed << std::setprecision(3) << r.ipc << ",\n";
    std::cout << "  \"backend_occupancy_pct\": " << std::fixed << std::setprecision(1) << r.backendOccupancy * 100.0 << ",\n";
    std::cout << "  \"backend_throughput\": " << std::fixed << std::setprecision(3) << r.backendThroughput << ",\n";
    std::cout << "  \"cycle_attribution_pct\": {\n";
    std::cout << "    \"raw_hazard\": " << std::fixed << std::setprecision(1) << r.rawStallPct * 100.0 << ",\n";
    std::cout << "    \"branch_stall\": " << std::fixed << std::setprecision(1) << r.branchStallPct * 100.0 << ",\n";
    std::cout << "    \"memory_wait\": " << std::fixed << std::setprecision(1) << r.memWaitPct * 100.0 << ",\n";
    std::cout << "    \"structural\": " << std::fixed << std::setprecision(1) << r.structuralStallPct * 100.0 << ",\n";
    std::cout << "    \"idle\": " << std::fixed << std::setprecision(1) << r.idlePct * 100.0 << "\n";
    std::cout << "  },\n";
    std::cout << "  \"diagnostics\": [";
    for (size_t i = 0; i < r.diagnostics.size(); i++) {
        std::cout << (i == 0 ? "\n    \"" : "    \"") << r.diagnostics[i] << "\"";
        if (i + 1 < r.diagnostics.size()) std::cout << ",";
        std::cout << "\n";
    }
    std::cout << (r.diagnostics.empty() ? "" : "  ") << "]\n";
    std::cout << "}" << std::endl;
}

void runCommand(const CLI& cli) {
    std::string trace = cli.getOption("--trace");
    if (trace.empty()) {
        Logger::error("Missing required argument: --trace <file>");
        exit(1);
    }

    SimConfig cfg;
    if (!cli.getOption("--aluUnits").empty()) cfg.aluUnits = std::stoi(cli.getOption("--aluUnits"));
    if (!cli.getOption("--memPorts").empty()) cfg.memPorts = std::stoi(cli.getOption("--memPorts"));
    if (!cli.getOption("--numa").empty()) cfg.numaNodes = std::stoi(cli.getOption("--numa"));

    Simulator sim(cfg);
    SimResult res = sim.run(trace);

    if (cli.hasFlag("--json")) {
        printJSON(res);
    } else {
        // Professional Text Output
        std::cout << "\n" << std::string(60, '=') << "\n";
        std::cout << "         ARCHITECTURAL SIMULATION REPORT\n";
        std::cout << std::string(60, '=') << "\n";
        std::cout << "   - IPC:                 " << std::fixed << std::setprecision(3) << res.ipc << "\n";
        std::cout << "   - Backend Occupancy:   " << std::fixed << std::setprecision(1) << res.backendOccupancy * 100.0 << "%\n";
        std::cout << "   - Backend Throughput:  " << std::fixed << std::setprecision(3) << res.backendThroughput << " (Retired/Occupancy)\n";
        
        std::cout << "\nCycle Attribution:\n";
        std::cout << "     - RAW Hazard:  " << std::fixed << std::setprecision(1) << res.rawStallPct * 100.0 << "%\n";
        std::cout << "     - Memory Wait: " << std::fixed << std::setprecision(1) << res.memWaitPct * 100.0 << "%\n";
        std::cout << "     - Structural:  " << std::fixed << std::setprecision(1) << res.structuralStallPct * 100.0 << "%\n";
        std::cout << "     - Idle:        " << std::fixed << std::setprecision(1) << res.idlePct * 100.0 << "%\n";

        if (!res.diagnostics.empty()) {
            std::cout << "\n[DIAGNOSTICS]\n";
            for (const auto& d : res.diagnostics) {
                std::cout << "  -> " << d << "\n";
            }
        }
        std::cout << std::string(60, '=') << "\n" << std::endl;
    }
}
