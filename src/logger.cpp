#include "logger.hpp"

LogLevel Logger::currentLevel = LogLevel::INFO_VAL;
bool Logger::quiet = false;

void Logger::setLevel(LogLevel level) {
    currentLevel = level;
}

void Logger::setQuiet(bool q) {
    quiet = q;
}

void Logger::info(const std::string& msg) {
    if (!quiet && currentLevel >= LogLevel::INFO_VAL)
        std::cout << "[INFO] " << msg << std::endl;
}

void Logger::debug(const std::string& msg) {
    if (!quiet && currentLevel >= LogLevel::DEBUG_VAL)
        std::cout << "[DEBUG] " << msg << std::endl;
}

void Logger::error(const std::string& msg) {
    // Error is never quiet unless explicitly muted for JSON
    if (!quiet)
        std::cerr << "[ERROR] " << msg << std::endl;
}
