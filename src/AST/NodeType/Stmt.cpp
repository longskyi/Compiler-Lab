#include "AST/NodeType/Stmt.h"


namespace AST {

unique_ptr<ASTNode> Assign::try_constructS(ASTNode * as , AbstractSyntaxTree * astTree) {
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
        // id = Expr ;
        assert(NonTnode->childs.size() == 4);
        assert(dynamic_cast<TermSymNode *>(NonTnode->childs[0].get())->token_type == u8"ID" && "不是id类型节点");
        assert(dynamic_cast<Expr *>(NonTnode->childs[2].get())&&"不是Expr类型节点");
        auto newNode = std::make_unique<Assign>();
        
        assert(dynamic_cast<TermSymNode *>(NonTnode->childs[0].get())->token_type == u8"ID" && "不是id类型节点");
        newNode->id_ptr = std::make_unique<SymIdNode>();
        newNode->id_ptr->Literal = static_cast<TermSymNode*>(NonTnode->childs[0].get())->value;
        
        newNode->expr_ptr.reset(static_cast<Expr*>(NonTnode->childs[2].release()));
        return newNode;
    }
    default:
        std::cerr <<"Not implement Assign Node :"<< targetProd<<std::endl;
        return nullptr;
    }
}


unique_ptr<ASTNode> Branch::try_constructS(ASTNode * as , AbstractSyntaxTree * astTree) {
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
        // if ( Bool ) Stmt
        assert(NonTnode->childs.size() == 5);
        assert(dynamic_cast<ASTBool *>(NonTnode->childs[2].get()) && "不是Bool类型节点");
        assert(dynamic_cast<Stmt *>(NonTnode->childs[4].get())&&"不是Stmt类型节点");
        auto newNode = std::make_unique<Branch>();
        
        newNode->bool_ptr.reset(static_cast<ASTBool*>(NonTnode->childs[2].release()));
        newNode->if_stmt_ptr.reset(static_cast<Stmt*>(NonTnode->childs[4].release()));
        newNode->op = BranchType::ifOnly;
        return newNode;
    }
    case 1:
    {
        // if ( Bool ) Stmt else Stmt
        assert(NonTnode->childs.size() == 7);
        assert(dynamic_cast<ASTBool *>(NonTnode->childs[2].get()) && "不是Bool类型节点");
        assert(dynamic_cast<Stmt *>(NonTnode->childs[4].get())&&"不是Stmt类型节点");
        assert(dynamic_cast<Stmt *>(NonTnode->childs[6].get())&&"不是Stmt类型节点");
        auto newNode = std::make_unique<Branch>();
        
        newNode->bool_ptr.reset(static_cast<ASTBool*>(NonTnode->childs[2].release()));
        newNode->if_stmt_ptr.reset(static_cast<Stmt*>(NonTnode->childs[4].release()));
        newNode->else_stmt_ptr = nullptr;
        newNode->else_stmt_ptr.value().reset(static_cast<Stmt*>(NonTnode->childs[6].release()));
        newNode->op = BranchType::ifelse;
        return newNode;
    }
    default:
        std::cerr <<"Not implement branch Node :"<< targetProd<<std::endl;
        return nullptr;
    }
}

unique_ptr<ASTNode> Loop::try_constructS(ASTNode * as , AbstractSyntaxTree * astTree) {
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
        // while ( Bool ) Stmt
        assert(NonTnode->childs.size() == 5);
        assert(dynamic_cast<ASTBool *>(NonTnode->childs[2].get()) && "不是Bool类型节点");
        assert(dynamic_cast<Stmt *>(NonTnode->childs[4].get())&&"不是Stmt类型节点");
        auto newNode = std::make_unique<Loop>();
        
        newNode->bool_ptr.reset(static_cast<ASTBool*>(NonTnode->childs[2].release()));
        newNode->stmt_ptr.reset(static_cast<Stmt*>(NonTnode->childs[4].release()));
        newNode->op = LoopType::whileLOOP;
        return newNode;
    }
    default:
        std::cerr <<"Not implement Loop Node :"<< targetProd<<std::endl;
        return nullptr;
    }
}


}