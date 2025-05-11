#include "SyntaxType.h"

// Symbol implementation
Symbol::Symbol(std::u8string sym, std::u8string type, 
       std::u8string pattern, bool isTerminal, size_t index)
    : sym_(std::move(sym)), tokentype_(std::move(type)),
      pattern_(std::move(pattern)), isTerminal_(isTerminal),
      index_(index) {}

const std::u8string& Symbol::sym() const { return sym_; }
const std::u8string& Symbol::tokentype() const { return tokentype_; }
const std::u8string& Symbol::pattern() const { return pattern_; }
bool Symbol::is_terminal() const { return isTerminal_; }
size_t Symbol::index() const { return index_; }

// SymbolTable implementation
const Symbol& SymbolTable::add_symbol(std::u8string sym, std::u8string type,
                 std::u8string pattern, bool isTerminal) {
    if (name_to_index_.count(sym)) {
        throw std::runtime_error("Symbol already exists");
    }
    size_t id = next_index_++;
    Symbol symb(sym, type, pattern, isTerminal, id);
    symbols_.push_back(symb);
    name_to_index_.emplace(std::move(sym), id);
    return symbols_.back();
}

const Symbol& SymbolTable::operator[](size_t index) const {
    if (index >= symbols_.size()) {
        throw std::out_of_range("Invalid symbol index");
    }
    return symbols_[index];
}

std::optional<size_t> SymbolTable::find_index(const std::u8string& name) const {
    auto it = name_to_index_.find(name);
    return it != name_to_index_.end() ? std::optional(it->second) : std::nullopt;
}

// Production implementation
Production::Production(size_t lhs_index, std::vector<size_t> rhs_indices, size_t index)
    : lhs_index_(lhs_index), rhs_indices_(rhs_indices), index_(index) {}

Production Production::create(const SymbolTable& symtab,
                       const std::u8string& lhs,
                       const std::vector<std::u8string>& rhs) {
    auto lhs_idx = symtab.find_index(lhs);
    if (!lhs_idx) {
        throw std::runtime_error("Undefined symbol: " + std::string(lhs.begin(), lhs.end()));
    }
    std::vector<size_t> rhs_indices;
    if(rhs.size()>0 && rhs[0] == u8"epsilon") {
        //epsilon production
        return Production{*lhs_idx, std::move(rhs_indices), next_index_++};
    }
    for (const auto& name : rhs) {
        auto idx = symtab.find_index(name);
        if (!idx) {
            throw std::runtime_error("Undefined symbol: " + std::string(name.begin(), name.end()));
        }
        rhs_indices.push_back(*idx);
    }
    return Production{*lhs_idx, std::move(rhs_indices), next_index_++};
}

size_t Production::lhs() const { return lhs_index_; }
const std::vector<size_t>& Production::rhs() const { return rhs_indices_; }
size_t Production::index() const { return index_; }