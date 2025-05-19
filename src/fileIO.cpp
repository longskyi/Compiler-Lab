#include"fileIO.h"
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
            symtable.add_symbol(nt, nt , u8"", false);
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


namespace LCMPFileIO {
    
    namespace fs = std::filesystem;

    struct StrProduction {
        std::string lhs;
        std::vector<std::string> rhs;
        std::string sym;
    };
    void copyToU8strProduction (U8StrProduction & dst ,const StrProduction & src) {
        dst.lhs = toU8str(src.lhs);
        for(const auto & rhsi : src.rhs) {
            dst.rhs.emplace_back(toU8str(rhsi));
        }
    }
    // 第一道处理：移除注释
    std::string removeComments(const std::string& line) {
        size_t commentPos = line.find("//");
        if (commentPos != std::string::npos) {
            return line.substr(0, commentPos);
        }
        return line;
    }

    // 第二道处理：解析产生式
    StrProduction parseProductionR(const std::string& line) {
        StrProduction prod;
        auto trim = [](std::string& s) {
            s.erase(s.find_last_not_of(" \t") + 1);
            s.erase(0, s.find_first_not_of(" \t"));
        };
        // 分割产生式和终结符部分
        size_t separatorPos = line.find('\\');
        if (separatorPos == std::string::npos) {
            std::cerr << "Invalid format: missing separator '\\'" << std::endl;
            return prod;
        }
        
        // 提取终结符部分（去除前后空格）
        prod.sym = line.substr(separatorPos + 1);
        trim(prod.sym);  // 假设有trim函数去除前后空格
        
        // 处理产生式部分
        std::string productionPart = line.substr(0, separatorPos);
        trim(productionPart);
        
        // 解析产生式
        size_t arrowPos = productionPart.find("->");
        if (arrowPos == std::string::npos) {
            std::cerr << "Invalid format: missing '->'" << std::endl;
            return prod;
        }
        
        // 提取左侧符号(LHS)
        prod.lhs = productionPart.substr(0, arrowPos);
        trim(prod.lhs);
        
        // 提取右侧符号(RHS)，按空格分割
        std::string rhsPart = productionPart.substr(arrowPos + 2);
        trim(rhsPart);
        
        // 分割右侧符号，保留空格分隔的原始结构
        std::istringstream rhsStream(rhsPart);
        std::string token;
        while (rhsStream >> token) {
            prod.rhs.push_back(token);
        }
        
        return prod;
    }


    StrProduction parseProduction(const std::string& line) {
                StrProduction prod;
        auto trim = [](std::string& s) {
            s.erase(s.find_last_not_of(" \t") + 1);
            s.erase(0, s.find_first_not_of(" \t"));
        };
    
        // 处理产生式部分
        std::string productionPart = line;
        trim(productionPart);
        
        // 解析产生式
        size_t arrowPos = productionPart.find("->");
        if (arrowPos == std::string::npos) {
            std::cerr << "Invalid format: missing '->'" << std::endl;
            return prod;
        }
        
        // 提取左侧符号(LHS)
        prod.lhs = productionPart.substr(0, arrowPos);
        trim(prod.lhs);
        
        // 提取右侧符号(RHS)，按空格分割
        std::string rhsPart = productionPart.substr(arrowPos + 2);
        trim(rhsPart);
        
        // 分割右侧符号，保留空格分隔的原始结构
        std::istringstream rhsStream(rhsPart);
        std::string token;
        while (rhsStream >> token) {
            prod.rhs.push_back(token);
        }
        
        return prod;
    }

    U8StrProduction parseProduction(const std::u8string_view& line) {
        auto q = parseProduction(toString(line));
        U8StrProduction tmp;
        copyToU8strProduction(tmp,q);
        return std::move(tmp);
    }

    ForceReducedProd parseProductionR(const std::u8string& line) {
        auto q = parseProductionR(toString(line));
        U8StrProduction tmp;
        copyToU8strProduction(tmp,q);
        ForceReducedProd ret;
        ret.strProd = std::move(tmp);
        ret.sym = toU8str(q.sym);
        return std::move(ret);
    }
    //have except
    std::vector<ForceReducedProd> parseProdFileR (const fs::path& inputPath) {

        if (!fs::exists(inputPath)) {
            std::cerr << "Error: File does not exist: " << inputPath << std::endl;
            throw std::runtime_error("File Not Found");
        }
        std::ifstream inputFile(inputPath);
        if (!inputFile.is_open()) {
            std::cerr << "Error opening file: " << inputPath << std::endl;
            throw std::runtime_error("Cannot Open File");
        }
        std::vector<ForceReducedProd> productions;
        std::string line;

        // 第一道处理：移除注释
        std::vector<std::string> cleanedLines;
        while (std::getline(inputFile, line)) {
            std::string cleaned = removeComments(line);
            if (!cleaned.empty()) {
                cleanedLines.push_back(cleaned);
            }
        }

        // 第二道处理：解析产生式
        for (const auto& cleanedLine : cleanedLines) {
            ForceReducedProd prod = parseProductionR(toU8str(cleanedLine));
            if (!prod.strProd.lhs.empty()) {
                productions.emplace_back(std::move(prod));
            }
        }
        return productions;
    }


    int test_main_strProd(const fs::path& inputPath) {
        // 检查文件是否存在
        if (!fs::exists(inputPath)) {
            std::cerr << "Error: File does not exist: " << inputPath << std::endl;
            return 1;
        }

        // 检查是否是常规文件
        if (!fs::is_regular_file(inputPath)) {
            std::cerr << "Error: Not a regular file: " << inputPath << std::endl;
            return 1;
        }

        std::ifstream inputFile(inputPath);
        if (!inputFile.is_open()) {
            std::cerr << "Error opening file: " << inputPath << std::endl;
            return 1;
        }

        std::vector<StrProduction> productions;
        std::string line;

        // 第一道处理：移除注释
        std::vector<std::string> cleanedLines;
        while (std::getline(inputFile, line)) {
            std::string cleaned = removeComments(line);
            if (!cleaned.empty()) {
                cleanedLines.push_back(cleaned);
            }
        }

        // 第二道处理：解析产生式
        for (const auto& cleanedLine : cleanedLines) {
            StrProduction prod = parseProductionR(cleanedLine);
            if (!prod.lhs.empty()) {
                productions.push_back(prod);
            }
        }

        // 输出结果
        for (const auto& prod : productions) {
            std::cout << "LHS: " << prod.lhs << "\nRHS: [";
            for (size_t i = 0; i < prod.rhs.size(); ++i) {
                if (i != 0) std::cout << ", ";
                std::cout << prod.rhs[i];
            }
            std::cout << "]\nSymbol: " << prod.sym << "\n\n";
        }

        return 0;
    }

}



inline void LCMPFileIO::JsonConverter::deserializeP(const nlohmann::json& j, SymbolTable& st) {
    j.at("next_index").get_to(st.next_index_);
    j.at("nonTerminalId").get_to(st.nonTerminalId);
    j.at("name_to_index").get_to(st.name_to_index_);
    std::vector<nlohmann::json> symbols;    //提供默认构造函数
    j.at("symbols").get_to(symbols);
    for(auto & p : symbols) {
        Symbol a(u8"",u8"",u8"",false,0);
        from_json(p,a);
        st.symbols_.emplace_back(std::move(a));
    }
}


void LCMPFileIO::JsonConverter::serialize(json& j, const SymbolTable& st) {
    j = {
    {"next_index", st.next_index_},
    {"nonTerminalId", st.nonTerminalId},
    {"name_to_index",st.name_to_index_},
    {"symbols",st.symbols_}
    };
}

void LCMPFileIO::JsonConverter::serialize(json& j, const Production & obj) {
    j = {
        {"index",obj.index_},
        {"lhs_index",obj.lhs_index_},
        {"next_index",obj.next_index_},
        {"rhs_indices",obj.rhs_indices_},
    };
}

void LCMPFileIO::JsonConverter::serialize(json& j, const dotProdc & obj) {
    j = {
        {"dot_pos",obj.dot_pos},
        {"prodcId",obj.producId},
    };
}


void from_json(const nlohmann::json& j, ASTbaseContent& obj) {
    j.at("gotoTable").get_to(obj.gotoTable);
    j.at("actionTable").get_to(obj.actionTable);
    j.at("states").get_to(obj.states);
    j.at("symtab").get_to(obj.symtab);
    std::unordered_map<ProductionId, nlohmann::json> prods;
    j.at("Productions").get_to(prods);
    obj.Productions.clear();
    NonTerminalId lhs_index_;
    std::vector<SymbolId> rhs_indices_;
    ProductionId index_;
    auto prod = LCMPFileIO::JsonConverter::Production_Construct_helper(lhs_index_,rhs_indices_,index_);
    for(auto p : prods) {
        from_json(p.second,prod);
        obj.Productions.insert(std::make_pair(p.first,prod));
    }
}


void from_json(const nlohmann::json& j, Production& obj) {
    NonTerminalId lhs_index_;
    std::vector<SymbolId> rhs_indices_;
    ProductionId index_;
    size_t next_index_;
    j.at("index").get_to(index_);
    j.at("lhs_index").get_to(lhs_index_);
    //j.at("next_index").get_to(next_index_);
    j.at("rhs_indices").get_to(rhs_indices_);
    obj = LCMPFileIO::JsonConverter::Production_Construct_helper(lhs_index_,rhs_indices_,index_);
}

void to_json(nlohmann::json & j, const action & obj) {
    auto visitor = overload {
        [&j](ProductionId v) {
            j = {{"type",1},{"val",v}};
        },
        [&j](StateId v) {
            j = {{"type",2},{"val",v}};
        }
    };
    std::visit(visitor,obj);
}

void from_json(const nlohmann::json& j, action& obj) {
    int type = j.at("type").get<int>();
    if (type == 1) {
        obj = j.at("val").get<ProductionId>();
    } else if (type == 2) {
        obj = j.at("val").get<StateId>();
    } else {
        throw std::runtime_error("Invalid action type");
    }
}

void to_json(nlohmann::json& j, const ASTbaseContent & obj) {
    j = {
    {"gotoTable",obj.gotoTable},
    {"Productions",obj.Productions},
    {"actionTable",obj.actionTable},
    {"states",obj.states},
    {"symtab",obj.symtab}};
}