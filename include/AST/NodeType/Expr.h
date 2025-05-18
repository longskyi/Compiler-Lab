#ifndef M_AST_EXPRTYPE_HEADER
#define M_AST_EXPRTYPE_HEADER
#include"AST/NodeType/ASTbaseType.h"

namespace AST
{

class Expr : public ASTNode
{
private:
    /* data */
public:
    Expr() {
        this->Ntype = ASTType::Expr;
        this->subType = ASTSubType::Expr;
    }
    ~Expr() {}
};


class ConstExpr : public Expr
{
private:
    /* data */
public:
    ConstExpr() {
        this->Ntype = ASTType::Expr;
        this->subType = ASTSubType::ConstExpr;
    }
    ~ConstExpr() {}
};

class DerefExpr : public Expr {};

class CallExpr : public Expr {};

class ArithExpr : public Expr {};



}
#endif