#ifndef LCMP_SYNTAXTYPE_HEADER
#define LCMP_SYNTAXTYPE_HEADER

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>
#include <stdexcept>
#include <cstddef>

class SymbolTable;

class Symbol {
friend class SymbolTable;
private:
    std::u8string sym_;
    std::u8string tokentype_;
    std::u8string pattern_;
    bool isTerminal_;
    size_t index_;

    Symbol(std::u8string sym, std::u8string type, 
           std::u8string pattern, bool isTerminal, size_t index);

public:
    Symbol(const Symbol&) = default;
    Symbol& operator=(const Symbol&) = default;
    
    const std::u8string& sym() const;
    const std::u8string& tokentype() const;
    const std::u8string& pattern() const;
    bool is_terminal() const;
    size_t index() const;
};

class SymbolTable {
private:
    std::vector<Symbol> symbols_;
    std::unordered_map<std::u8string, size_t> name_to_index_;
    size_t next_index_ = 0;

public:
    const Symbol& add_symbol(std::u8string sym, std::u8string type,
                     std::u8string pattern, bool isTerminal);
    const Symbol& operator[](size_t index) const;
    std::optional<size_t> find_index(const std::u8string& name) const;
    inline const auto & symbols() { return symbols_; } 
};

class Production {
private:
    size_t lhs_index_;
    std::vector<size_t> rhs_indices_;
    size_t index_;
    
    inline static size_t next_index_ = 0;
    Production(size_t lhs_index, std::vector<size_t> rhs_indices, size_t index);

public:
    Production(const Production&) = default;
    Production& operator=(const Production&) = default;
    Production(Production&&) = default;
    Production& operator=(Production&&) = default;

    static Production create(const SymbolTable& symtab,
                           const std::u8string& lhs,
                           const std::vector<std::u8string>& rhs);

    size_t lhs() const;
    const std::vector<size_t>& rhs() const;
    size_t index() const;
};

#endif // SYNTAXTYPE_HEADER