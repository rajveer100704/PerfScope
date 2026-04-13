#include "cli.hpp"
#include "commands.hpp"
#include "logger.hpp"
#include <iostream>
#include <iomanip>

CLI::CLI(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        args.push_back(argv[i]);
    }
    if (!args.empty()) {
        command = args[0];
    }
}

std::string CLI::getCommand() const {
    return command;
}

std::string CLI::getOption(const std::string& key) const {
    for (size_t i = 0; i < args.size(); i++) {
        if (args[i] == key && i + 1 < args.size()) {
            return args[i + 1];
        }
    }
    return "";
}

bool CLI::hasFlag(const std::string& key) const {
    for (const auto& arg : args) {
        if (arg == key) return true;
    }
    return false;
}

void CLI::execute() {
    if (hasFlag("--debug")) {
        Logger::setLevel(LogLevel::DEBUG_VAL);
    }
    if (hasFlag("--json") || hasFlag("--quiet")) {
        Logger::setQuiet(true);
    }

    if (command == "run") {
        runCommand(*this);
    } else if (command == "benchmark") {
        benchmarkCommand(*this);
    } else if (command == "analyze-system") {
        analyzeSystemCommand(*this);
    } else if (command == "sweep") {
        sweepCommand(*this);
    } else if (command == "inspect") {
        inspectCommand(*this);
    } else if (command == "help" || command == "--help" || command == "-h" || command.empty()) {
        printHelp();
    } else {
        Logger::error("Unknown command: " + command);
        printHelp();
        exit(1);
    }
}

void CLI::printHelp() const {
    std::cout << "\nPerformance Analysis-Oriented CPU Simulator\n";
    std::cout << "Usage: simulator <command> [options]\n\n";
    std::cout << "Commands:\n";
    std::cout << "  run              Run architectural simulation on a trace file\n";
    std::cout << "  benchmark        Run predefined workload suites\n";
    std::cout << "  analyze-system   Detect local hardware and run synthetic baseline\n";
    std::cout << "  sweep            Perform parameter sensitivity analysis\n";
    std::cout << "  inspect          Detailed cycle-by-cycle pipeline inspection\n";
    std::cout << "  help             Show this help message\n\n";
    std::cout << "Global Options:\n";
    std::cout << "  --json           Enable machine-readable output mode\n";
    std::cout << "  --debug          Enable verbose diagnostic logging\n";
    std::cout << "  --quiet          Suppress all non-result output\n\n";
}
