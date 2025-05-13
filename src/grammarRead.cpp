#include"grammarRead.h"
#include"SyntaxType.h"
#include"stringUtil.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <filesystem>

void readTerminals(SymbolTable& symtable, std::filesystem::path terminalPath) {
    std::ifstream file(terminalPath);
    if (!file.is_open()) {
        throw  std::runtime_error("Failed to open terminal file");
    }

    std::string line;
    while (std::getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line.find("//") == 0) {
            continue;
        }

        // Remove trailing comments
        size_t commentPos = line.find("//");
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }

        // Trim whitespace
        line.erase(line.find_last_not_of(" \t") + 1);
        line.erase(0, line.find_first_not_of(" \t"));

        if (line.empty()) continue;

        // Parse the line in format: "type","name","pattern" (or "type","name")
        std::vector<std::string> tokens;
        size_t start = 0;
        while (true) {
            size_t quote1 = line.find('"', start);
            if (quote1 == std::string::npos) break;

            size_t quote2 = line.find('"', quote1 + 1);
            if (quote2 == std::string::npos) break;

            tokens.push_back(line.substr(quote1 + 1, quote2 - quote1 - 1));
            start = quote2 + 1;
        }

        if (tokens.size() < 2) continue; // At least "type","name" required

        std::string type = tokens[0];
        std::string name = tokens[1];
        std::string pattern = (tokens.size() >= 3) ? tokens[2] : "";

        symtable.add_symbol(toU8str(name), toU8str(type), toU8str(pattern), true);
    }
}

void readProductionRule(SymbolTable& symtable, std::vector<Production>& productions, std::filesystem::path rulePath) {
    // First pass: collect all non-terminal symbols
    // 修剪lambda
    auto trim = [](std::string& s) {
        s.erase(s.find_last_not_of(" \t") + 1);
        s.erase(0, s.find_first_not_of(" \t"));
    };
    std::vector<std::u8string> nonTerminals;
    
    {
        std::ifstream file(rulePath);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open rule file");
        }

        std::string line;
        while (std::getline(file, line)) {
            // Skip empty lines and comments
            if (line.empty() || line.find("//") == 0) {
                continue;
            }

            // Remove trailing comments
            size_t commentPos = line.find("//");
            if (commentPos != std::string::npos) {
                line = line.substr(0, commentPos);
            }

            trim(line);

            if (line.empty()) continue;

            // Extract LHS (non-terminal)
            size_t arrowPos = line.find("->");
            if (arrowPos == std::string::npos) continue;

            std::string lhs = line.substr(0, arrowPos);
            // Trim whitespace from LHS
            trim(lhs);

            if (!lhs.empty()) {
                nonTerminals.push_back(toU8str(lhs));
            }
        }
    }

    // Add non-terminals to symbol table (if not already exists)
    for (const auto& nt : nonTerminals) {
        try {
            symtable.add_symbol(nt, u8"", u8"", false);
        } catch (const std::runtime_error&) {
            // Symbol already exists, skip
        }
    }

    // Second pass: parse productions
    std::ifstream file(rulePath);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open rule file");
    }

    std::string line;
    while (std::getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line.find("//") == 0) {
            continue;
        }

        // Remove trailing comments
        size_t commentPos = line.find("//");
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }

        // Trim whitespace
        trim(line);

        if (line.empty()) continue;

        // Parse production rule
        size_t arrowPos = line.find("->");
        if (arrowPos == std::string::npos) continue;

        std::string lhs = line.substr(0, arrowPos);
        // Trim whitespace from LHS
        trim(lhs);

        if (lhs.empty()) continue;

        std::string rhsPart = line.substr(arrowPos + 2);
        // Trim whitespace from RHS
        trim(rhsPart);
        // Split alternatives (using | but not \|)
        std::vector<std::string> alternatives;
        size_t start = 0;
        size_t pipePos;
        while ((pipePos = rhsPart.find('|', start)) != std::string::npos) {
            // Check if it's escaped
            if (pipePos > 0 && rhsPart[pipePos - 1] == '\\') {
                start = pipePos + 1;
                continue;
            }
            
            std::string alt = rhsPart.substr(start, pipePos - start);
            // Trim whitespace
            trim(alt);
            if (!alt.empty()) {
                alternatives.push_back(alt);
            }
            start = pipePos + 1;
        }
        
        // Add the last alternative
        std::string lastAlt = rhsPart.substr(start);
        trim(lastAlt);
        if (!lastAlt.empty()) {
            alternatives.push_back(lastAlt);
        }

        // Create productions for each alternative
        for (const auto& alt : alternatives) {
            std::vector<std::u8string> rhsSymbols;
            
            // Split RHS into symbols
            std::istringstream iss(alt);
            std::string symbol;
            while (iss >> symbol) {
                // Handle escaped | as a symbol
                if (symbol == "\\|") {
                    symbol = "|";
                }
                rhsSymbols.push_back(toU8str(symbol));
            }
            
            // Create the production
            productions.push_back(Production::create(symtable, toU8str(lhs), rhsSymbols));
        }
    }
}