#pragma once
#include <iostream>
#include <string>

enum class LogLevel {
    ERROR_VAL = 0,
    INFO_VAL = 1,
    DEBUG_VAL = 2
};

class Logger {
public:
    static void setLevel(LogLevel level);
    static void setQuiet(bool q);
    
    static void info(const std::string& msg);
    static void debug(const std::string& msg);
    static void error(const std::string& msg);

private:
    static LogLevel currentLevel;
    static bool quiet;
};
