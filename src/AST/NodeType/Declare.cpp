#include"AST/NodeType/Program.h"
#include<memory>
#include"AST/NodeType/ASTbaseType.h"
#include"AST/NodeType/Declare.h"

namespace AST
{

unique_ptr<ASTNode> FuncDeclare::try_constructS(ASTNode * as , AbstractSyntaxTree * astTree) {
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
        //Decl -> Type id ( ArgList ) Block
        assert(NonTnode->childs.size() == 6);
        assert(dynamic_cast<pType *>(NonTnode->childs[0].get()) && "是基本类型节点");
        assert(dynamic_cast<TermSymNode*>(NonTnode->childs[1].get()));
        assert(dynamic_cast<TermSymNode*>(NonTnode->childs[1].get())->token_type == u8"ID");
        assert(dynamic_cast<ArgList*>(NonTnode->childs[3].get()));
        assert(dynamic_cast<Block*>(NonTnode->childs[5].get()));
        auto newNode = std::make_unique<FuncDeclare>();
        newNode->funcRetType = (static_cast<pType*>(NonTnode->childs[0].get()))->type;
        newNode->id_ptr = std::make_unique<SymIdNode>();
        newNode->id_ptr->Literal = static_cast<TermSymNode*>(NonTnode->childs[1].get())->value;
        newNode->ArgList_ptr.reset(static_cast<ArgList *>(NonTnode->childs[3].release()));
        newNode->Block_ptr.reset(static_cast<Block *>(NonTnode->childs[5].release()));
        return newNode;
    }
    default:
        std::cerr <<"Not implement FuncDeclare Node :"<< targetProd<<std::endl;
        return nullptr;
    }
}    

void FuncDeclare::accept(ASTVisitor& visitor){
    visitor.enter(this);
    id_ptr->accept(visitor);
    visitor.visit(this);
    ArgList_ptr->accept(visitor);
    Block_ptr->accept(visitor);
    visitor.quit(this);
    return;
}
} // namespace AST
