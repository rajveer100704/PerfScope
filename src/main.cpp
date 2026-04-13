#include "cli.hpp"
#include <iostream>

/**
 * CPU Simulator Analysis Toolkit
 * 
 * Provides a cycle-accurate architectural analysis of CPU pipeline behavior,
 * branch prediction efficiency, and memory hierarchy effects.
 */
int main(int argc, char* argv[]) {
    try {
        CLI cli(argc, argv);
        cli.execute();
    } catch (const std::exception& e) {
        std::cerr << "[CRITICAL ERROR] " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "[CRITICAL ERROR] Unknown exception occurred." << std::endl;
        return 1;
    }
    
    return 0;
}
