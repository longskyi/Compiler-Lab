#include"AST/NodeType/Param.h"
#include<memory>
#include"AST/NodeType/ASTbaseType.h"


namespace AST
{
    
unique_ptr<ASTNode> Param::try_constructS(ASTNode * as , AbstractSyntaxTree * astTree) {
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
        // Expr
        assert(NonTnode->childs.size() == 1);
        assert(dynamic_cast<Expr *>(NonTnode->childs[0].get()) && "不是Expr类型节点");

        auto newNode = std::make_unique<Param>();
        newNode->expr_ptr.reset(static_cast<Expr *>(NonTnode->childs[0].release()));
        newNode->op = ParamOp::expr;
        return newNode;
    }
    case 1:
    {
        // id [ ]
        assert(NonTnode->childs.size() == 1);
        assert(dynamic_cast<SymIdNode *>(NonTnode->childs[0].get()) && "不是sym类型节点");

        auto newNode = std::make_unique<Param>();
        newNode->id_ptr.reset(static_cast<SymIdNode *>(NonTnode->childs[0].release()));
        newNode->op = ParamOp::ARRAY_PTRP;
        return newNode;
    }
    case 2:
    {
        // id ( )
        assert(NonTnode->childs.size() == 1);
        assert(dynamic_cast<SymIdNode *>(NonTnode->childs[0].get()) && "不是sym类型节点");

        auto newNode = std::make_unique<Param>();
        newNode->id_ptr.reset(static_cast<SymIdNode *>(NonTnode->childs[0].release()));
        newNode->op = ParamOp::FUNC_PTRP;
        return newNode;
    }
    default:
        std::cerr <<"Not implement Param Node :"<< targetProd<<std::endl;
        return nullptr;
    }
}


void Param::accept(ASTVisitor & visitor) {
    visitor.enter(this);
    if(this->op == ParamOp::expr) {
        this->expr_ptr->accept(visitor);
    }
    else if(this->op == ParamOp::ARRAY_PTRP || this->op == ParamOp::FUNC_PTRP) {
        this->id_ptr->accept(visitor);
    }
    visitor.visit(this);
    visitor.quit(this);
    return;
}

} // namespace name