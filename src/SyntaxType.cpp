#include "SyntaxType.h"


// SymbolTable implementation
const Symbol& SymbolTable::add_symbol(std::u8string sym, std::u8string type,
                 std::u8string pattern, bool isTerminal) {
    if (name_to_index_.count(sym)) {
        throw std::runtime_error("Symbol already exists");
    }
    size_t id = next_index_++;
    Symbol symb(sym, type, pattern, isTerminal, id);
    symbols_.push_back(symb);
    auto ntid = NonTerminalId(id);
    if(! isTerminal) nonTerminalId.push_back(ntid);
    name_to_index_.emplace(std::move(sym), id);
    return symbols_.back();
}


std::optional<SymbolId> SymbolTable::find_index(const std::u8string& name) const {
    auto it = name_to_index_.find(name);
    return it != name_to_index_.end() ? std::optional(SymbolId(it->second)) : std::nullopt;
}

// Production implementation
Production::Production(NonTerminalId lhs_index, std::vector<SymbolId> rhs_indices, size_t index)
    : lhs_index_(lhs_index), rhs_indices_(rhs_indices), index_(ProductionId(index)) {}

Production Production::create(const SymbolTable& symtab,
                       const std::u8string& lhs,
                       const std::vector<std::u8string>& rhs) {
    auto lhs_idx = symtab.find_index(lhs);
    if (!lhs_idx) {
        throw std::runtime_error("Undefined symbol: " + std::string(lhs.begin(), lhs.end()));
    }
    if(symtab[lhs_idx.value()].is_terminal()) {
        throw std::runtime_error("ERROR PRODUCTION WITH TERMINAL lHS");
    }
    std::vector<SymbolId> rhs_indices;
    if(rhs.size()>0 && rhs[0] == u8"epsilon") {
        //epsilon production
        return Production{NonTerminalId(*lhs_idx), std::move(rhs_indices), next_index_++};
    }
    for (const auto& name : rhs) {
        auto idx = symtab.find_index(name);
        if (!idx) {
            throw std::runtime_error("Undefined symbol: " + std::string(name.begin(), name.end()));
        }
        rhs_indices.push_back(SymbolId(*idx));
    }
    return Production{NonTerminalId(*lhs_idx), std::move(rhs_indices), next_index_++};
}

