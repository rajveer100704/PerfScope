#pragma once
#include <string>
#include <vector>
#include <map>

class CLI {
public:
    CLI(int argc, char* argv[]);
    void execute();

    std::string getCommand() const;
    std::string getOption(const std::string& key) const;
    bool hasFlag(const std::string& key) const;
    
    // Help and Validation
    void printHelp() const;
    void validateRunArgs() const;

private:
    std::vector<std::string> args;
    std::string command;
};
