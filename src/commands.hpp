#pragma once
#include "cli.hpp"

// Command handlers for the simulator CLI
void runCommand(const CLI& cli);
void benchmarkCommand(const CLI& cli);
void analyzeSystemCommand(const CLI& cli);
void sweepCommand(const CLI& cli);
void inspectCommand(const CLI& cli);
