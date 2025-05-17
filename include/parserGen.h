#ifndef LCMP_PARSERGEN_HEADER
#define LCMP_PARSERGEN_HEADER
#include"SyntaxType.h"

void parserGen_test_main();

typedef struct ASTbaseContent {
    std::vector<std::vector<dotProdc>> states;
    std::vector<std::unordered_map<SymbolId, StateId>> gotoTable;
    std::vector<std::unordered_map<SymbolId, action>> actionTable;
    SymbolTable symtab;
    std::unordered_map<ProductionId, Production> Productions;
}ASTbaseContent;

ASTbaseContent parserGen(std::filesystem::path grammarFile , std::filesystem::path terminalsFile , std::filesystem::path SLRruleFile);
std::u8string formatProduction(const Production& prod, size_t dot_pos, const SymbolTable& symtab);

#endif