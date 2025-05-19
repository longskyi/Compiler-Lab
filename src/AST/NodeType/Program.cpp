#include"AST/NodeType/Program.h"
#include<memory>
#include"AST/NodeType/ASTbaseType.h"

namespace AST
{

unique_ptr<ASTNode> Program::try_constructS(ASTNode * as , AbstractSyntaxTree * astTree) {
    auto * NonTnode = dynamic_cast<NonTermProdNode *>(as);
    if(!NonTnode) {
        std::cerr << "Compiler internal Error ptype 不接受非NonTermProdNode的try contrust请求";
        return nullptr;
        //throw std::runtime_error("ERROR,ConstExpr不接受非NonTermProdNode的try contrust请求");
    }
    //检查 str prod匹配
    int targetProd = 0;
    for(targetProd = 0 ; targetProd < SupportProd.size() ; targetProd++) {
        U8StrProduction u8prod = LCMPFileIO::parseProduction(SupportProd[targetProd]);
        if(equals(u8prod,astTree->Productions.at(NonTnode->prodId),astTree->symtab)) {
            break;
        }
    }
    if(targetProd == SupportProd.size()) {
        return nullptr;
    }
    switch (targetProd)
    {
    case 0:
    {
        //
        assert(NonTnode->childs.size() == 1);
        assert(dynamic_cast<BlockItemList *>(NonTnode->childs[0].get()));
        
        auto  newNode = std::make_unique<Program>();
        newNode->ItemList.reset(static_cast<BlockItemList*>(NonTnode->childs[0].release()));
        return newNode;
    }
    default:
        std::cerr <<"Not implement BlockItem Node :"<< targetProd<<std::endl;
        return nullptr;
    }
}
void Program::accept(ASTVisitor& visitor) {
    visitor.enter(this);
    visitor.visit(this);
    ItemList->accept(visitor);
    visitor.quit(this);
    return;
}
}

