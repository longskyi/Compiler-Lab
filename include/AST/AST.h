#ifndef LCMP_AST_HEADER
#define LCMP_AST_HEADER
#include <memory>
#include <string>
#include <stack>
#include <stdexcept>
#include <fstream>
#include "templateUtil.h"
#include "SyntaxType.h"
#include "lexer.h"
#include"parserGen.h"
#include"stringUtil.h"
#include"fileIO.h"

#include"AST/NodeType/ASTbaseType.h"
#include"AST/NodeType/Expr.h"
#include"AST/NodeType/Program.h"
#include"AST/NodeType/Arg.h"
#include"AST/NodeType/ASTBool.h"
#include"AST/NodeType/Declare.h"
#include"AST/NodeType/Param.h"
#include"AST/NodeType/Stmt.h"
#include"AST/NodeType/Type.h"

namespace AST
{



extern StateId startStateId;


void printCommonAST(const unique_ptr<ASTNode>& node, int depth = 0);

void AST_test_main();

inline unique_ptr<ASTNode> AST_specified_node_construct(unique_ptr<NonTermProdNode> prodNode , AbstractSyntaxTree * ast_tree) {
    unique_ptr<ASTNode> ret;
    ret = Program::try_constructS(prodNode.get(),ast_tree);
    if(ret) {
        return ret;
    } 
    ret = BlockItem::try_constructS(prodNode.get(),ast_tree);
    if(ret) {
        return ret;
    }
    ret = BlockItemList::try_constructS(prodNode.get(),ast_tree);
    if(ret) {
        return ret;
    }
    ret = IdDeclare::try_constructS(prodNode.get(),ast_tree);
    if(ret) {
        return ret;
    }
    ret = ConstExpr::try_constructS(prodNode.get(),ast_tree);
    if(ret) {
        return ret;
    }
    ret = pType::try_constructS(prodNode.get(),ast_tree);
    if(ret) {
        return ret;
    }
    std::cerr <<"构造失败"<< toString_view(LCMPFileIO::formatProduction(ast_tree->Productions.at(prodNode->prodId),ast_tree->symtab));
    
    return nullptr;
}


class ASTEnumTypeVisitor : public ASTVisitor {
int depth = 0;
public:
    bool moveSequence = false;

    virtual void visit(ASTNode* node) {
        if(moveSequence) {
            std::cout<<ASTSubTypeToString(node->subType)<<std::endl;   
        }
        return;
    }
    inline virtual void enter(ASTNode* node) {
        
        if(!moveSequence) {
            for(int i = 0 ;i <depth;i++) {
                std::cout<<" ";
            }
            std::cout<<ASTSubTypeToString(node->subType)<<std::endl;
        }
        depth++;
        return;
    }
    inline virtual void quit(ASTNode* node) {
        depth--;
        return;
    }
};


} // namespace AST


#endif