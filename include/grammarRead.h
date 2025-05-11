#ifndef LCMP_GRAMMAR_READ_HEADER
#define LCMP_GRAMMAR_READ_HEADER

#include"SyntaxType.h"
#include<filesystem>

void readTerminals(SymbolTable & symtable,std::filesystem::path terminalPath);

void readProductionRule(SymbolTable & symtable,std::vector<Production> & productions , std::filesystem::path rulePath);

#endif //LCMP_GRAMMAR_READ_HEADER