#ifndef M_AST_EXPR_BASE_HEADER
#define M_AST_EXPR_BASE_HEADER
#include "SyntaxType.h"
#include "lcmpfileio.h"
#include "AST/NodeType/ASTbaseType.h"
#include <memory>

namespace AST {
class Expr : public ASTNode
{
public:
    static constexpr std::array<std::u8string_view,1> SupportProd=
    {u8"Expr -> (Expr)"};
    Expr() {
        this->Ntype = ASTType::Expr;
        this->subType = ASTSubType::Expr;
    }
    ~Expr() {}
    inline static unique_ptr<ASTNode> try_constructS(ASTNode * as , AbstractSyntaxTree * astTree) {
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
            // ( Expr )
            assert(NonTnode->childs.size() == 3);
            assert(dynamic_cast<Expr *>(NonTnode->childs[1].get()) && "不是Expr类型节点");
            
            unique_ptr<Expr> stoleNode;
            stoleNode.reset(static_cast<Expr *>(NonTnode->childs[1].release()));
            return stoleNode;
        }
        default:
            std::cerr <<"Not implement Expr Node :"<< targetProd<<std::endl;
            return nullptr;
        }
    }
    unique_ptr<ASTNode> try_construct(ASTNode * as , AbstractSyntaxTree * astTree) override {
        return Expr::try_constructS(as,astTree);
    }
    inline void accept(ASTVisitor & visitor) override {
        visitor.enter(this);
        visitor.visit(this);        
        visitor.quit(this);
        return;
    }
};

class Stmt : public ASTNode 
{
public:
    inline Stmt() {
        this->Ntype = ASTType::Stmt;
    }
};

class Declare : public ASTNode 
{
public:
    Declare() {
        this->Ntype = ASTType::Declare;
        this->subType = ASTSubType::Declare;
    }
};

}

#endif