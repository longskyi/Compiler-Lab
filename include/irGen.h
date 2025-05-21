#ifndef LCMP_IR_GEN_HEADER
#define LCMP_IR_GEN_HEADER
#include "AST/AST.h"
#include "Semantic.h"
//生成IR
class IRGenVisitor : public AST::ASTVisitor {
public:
    Semantic::SemanticSymbolTable * rootTable;
    Semantic::SemanticSymbolTable * currentTable;
    
};


#endif