#include"AST/NodeType/Expr.h"
#include"AST/NodeType/Param.h"

namespace AST
{


unique_ptr<ASTNode> ConstExpr::try_constructS(ASTNode * as , AbstractSyntaxTree * astTree) {
    auto * NonTnode = dynamic_cast<NonTermProdNode *>(as);
    if(!NonTnode) {
        std::cerr << "Compiler internal Error ConstExpr不接受非NonTermProdNode的try contrust请求";
        return nullptr;
        //throw std::runtime_error("ERROR,ConstExpr不接受非NonTermProdNode的try contrust请求");
    }
    else {
        //检查 str prod匹配
        bool prodAccept = false;
        for(auto & prod : SupportProd) {
            U8StrProduction u8prod = LCMPFileIO::parseProduction(prod);
            if(equals(u8prod,astTree->Productions.at(NonTnode->prodId),astTree->symtab)) {
                prodAccept = true;
            }
        }
        if(!prodAccept) {
            return nullptr;
        }
        if(NonTnode->childs.size() != 1) {
            std::unreachable();
        }
        if(NonTnode->childs[0]->Ntype != ASTType::TermSym ) {
            std::unreachable();
        }
        unique_ptr <ConstExpr> newNode = std::make_unique<ConstExpr>();
        auto * TermNode = static_cast<TermSymNode *>(NonTnode->childs[0].get());
        if(TermNode->token_type == u8"NUM") {
            newNode->value = std::stoi(toString(TermNode->value));
            newNode->Type.basicType = baseType::INT;
        }
        else if(TermNode->token_type == u8"FLO") {
            newNode->value = std::stof(toString(TermNode->value));
            newNode->Type.basicType = baseType::FLOAT;
        }
        else {
            std::unreachable();
        }
        return newNode;
    }
}


unique_ptr<ASTNode> DerefExpr::try_constructS(ASTNode * as , AbstractSyntaxTree * astTree) {
    auto * NonTnode = dynamic_cast<NonTermProdNode *>(as);
    if(!NonTnode) {
        std::cerr << "Compiler internal Error Expr 不接受非NonTermProdNode的try contrust请求";
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
        // * Expr
        assert(NonTnode->childs.size() == 2);
        assert(dynamic_cast<Expr *>(NonTnode->childs[1].get()) && "不是Expr类型节点");
        
        auto newNode = std::make_unique<DerefExpr>();
        newNode->subExpr.reset(static_cast<Expr *>(NonTnode->childs[1].release()));
        return newNode;
    }
    default:
        std::cerr <<"Not implement Expr Node :"<< targetProd<<std::endl;
        return nullptr;
    }
}

unique_ptr<ASTNode> CallExpr::try_constructS(ASTNode * as , AbstractSyntaxTree * astTree) {
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
        // id ( ParamList )
        assert(NonTnode->childs.size() == 4);
        assert(dynamic_cast<SymIdNode *>(NonTnode->childs[0].get()) && "不是sym类型节点");
        assert(dynamic_cast<ParamList *>(NonTnode->childs[2].get()) && "不是paramlist类型节点");
        //assert(dynamic_cast<TermSymNode*>(NonTnode->childs[0].get())->token_type == u8"MUL");

        auto newNode = std::make_unique<CallExpr>();
        newNode->id_ptr.reset(static_cast<SymIdNode*>(NonTnode->childs[0].release()));
        newNode->ParamList_ptr.reset(static_cast<ParamList*>(NonTnode->childs[2].release()));
        return newNode;
    }
    default:
        std::cerr <<"Not implement CallExpr Node :"<< targetProd<<std::endl;
        return nullptr;
    }
}

void CallExpr::accept(ASTVisitor & visitor) {
    visitor.enter(this);
    this->id_ptr->accept(visitor);
    this->ParamList_ptr->accept(visitor);
    visitor.visit(this);
    visitor.quit(this);
}

unique_ptr<ASTNode> ArithExpr::try_constructS(ASTNode * as , AbstractSyntaxTree * astTree) {
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
        // Expr + Expr
        assert(NonTnode->childs.size() == 3);
        assert(dynamic_cast<Expr *>(NonTnode->childs[0].get()) && "不是Expr类型节点");
        assert(dynamic_cast<Expr *>(NonTnode->childs[2].get()) && "不是Expr类型节点");
        assert(dynamic_cast<TermSymNode*>(NonTnode->childs[1].get())->token_type == u8"ADD");

        auto newNode = std::make_unique<ArithExpr>();
        newNode->Lval_ptr.reset(static_cast<Expr *>(NonTnode->childs[0].release()));
        newNode->Rval_ptr.reset(static_cast<Expr *>(NonTnode->childs[2].release()));
        newNode->Op = BinaryOpEnum::ADD;
        return newNode;
    }
    case 1:
    {
        // Expr * Expr
        assert(NonTnode->childs.size() == 3);
        assert(dynamic_cast<Expr *>(NonTnode->childs[0].get()) && "不是Expr类型节点");
        assert(dynamic_cast<Expr *>(NonTnode->childs[2].get()) && "不是Expr类型节点");
        assert(dynamic_cast<TermSymNode*>(NonTnode->childs[1].get())->token_type == u8"MUL");

        auto newNode = std::make_unique<ArithExpr>();
        newNode->Lval_ptr.reset(static_cast<Expr *>(NonTnode->childs[0].release()));
        newNode->Rval_ptr.reset(static_cast<Expr *>(NonTnode->childs[2].release()));
        newNode->Op = BinaryOpEnum::MUL;
        return newNode;
    }
    default:
        std::cerr <<"Not implement ArithExpr Node :"<< targetProd<<std::endl;
        return nullptr;
    }
}

}