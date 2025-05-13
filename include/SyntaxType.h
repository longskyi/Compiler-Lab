#ifndef LCMP_SYNTAXTYPE_HEADER
#define LCMP_SYNTAXTYPE_HEADER
#include"templateUtil.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <stdexcept>
#include <cstddef>
#include <variant>


using ProductionId = StrongId<struct ProductionTag>;
using StateId = StrongId<struct StateTag>;
using NonTerminalId = StrongId<struct NonTerminalTag>;
using TerminalId = StrongId<struct TerminalTag>;
using SymbolId = StrongId<struct SymbolTag>;
//必须显式构造：Id id = Id(1); 可隐式转换到底层

template<typename T>
concept SymbolIdType = std::is_same_v<T, SymbolId> || 
                    std::is_same_v<T, TerminalId> || 
                    std::is_same_v<T, NonTerminalId>;


using action = std::variant<ProductionId,StateId>;
using utf8char = std::u8string; //约定

struct dotProdc
{
    size_t dot_pos;
    ProductionId producId;
};


class SymbolTable;

class Symbol {
friend class SymbolTable;
private:
    std::u8string sym_;
    std::u8string tokentype_;
    std::u8string pattern_;
    bool isTerminal_;
    size_t index_;

    inline Symbol(std::u8string sym, std::u8string type, 
       std::u8string pattern, bool isTerminal, size_t index)
    : sym_(std::move(sym)), tokentype_(std::move(type)),
      pattern_(std::move(pattern)), isTerminal_(isTerminal),
      index_(index) {}

public:
    Symbol(const Symbol&) = default;
    Symbol& operator=(const Symbol&) = default;

    inline const std::u8string& sym() const { return sym_; }
    inline const std::u8string& tokentype() const { return tokentype_; }
    inline const std::u8string& pattern() const { return pattern_; }
    inline bool is_terminal() const { return isTerminal_; }
    inline size_t index() const { return index_; }
};

class SymbolTable {
private:
    std::vector<Symbol> symbols_;
    std::vector<NonTerminalId>nonTerminalId;
    std::unordered_map<std::u8string, size_t> name_to_index_;
    size_t next_index_ = 0;

public:
    const Symbol& add_symbol(std::u8string sym, std::u8string type,
                     std::u8string pattern, bool isTerminal);
    const Symbol& operator[](SymbolIdType auto index) const {
        if (static_cast<size_t>(index) >= symbols_.size()) {
            throw std::out_of_range("Invalid symbol index");
        }
        return symbols_[static_cast<SymbolId>(index)];
    }
    std::optional<SymbolId> find_index(const std::u8string& name) const;
    inline const auto & symbols() { return symbols_; }
    inline const auto & nonTerminals() const {return nonTerminalId;}
};


class Production {
private:
    NonTerminalId lhs_index_;
    std::vector<SymbolId> rhs_indices_;
    ProductionId index_;
    
    inline static size_t next_index_ = 0;
    Production(NonTerminalId lhs_index, std::vector<SymbolId> rhs_indices, size_t index);

public:
    Production(const Production&) = default;
    Production& operator=(const Production&) = default;
    Production(Production&&) = default;
    Production& operator=(Production&&) = default;

    static Production create(const SymbolTable& symtab,
                           const std::u8string& lhs,
                           const std::vector<std::u8string>& rhs);

    inline NonTerminalId lhs() const { return lhs_index_; }
    inline const std::vector<SymbolId>& rhs() const { return rhs_indices_; }
    inline ProductionId index() const { return index_; }
};

#endif // SYNTAXTYPE_HEADER