#pragma once
#include "simulator_types.hpp"
#include "config.hpp"
#include <string>

class Simulator {
public:
    explicit Simulator(const SimConfig& config);
    
    SimResult run(const std::string& tracePath);
    
private:
    SimConfig config;
};
