#ifndef M_AST_EXPRTYPE_HEADER
#define M_AST_EXPRTYPE_HEADER
#include"AST/NodeType/ASTbaseType.h"

namespace AST
{

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
};

enum RightValueBehave {
    ref,
    direct,
    ptr_cul,
};

class RightValueExpr : public Expr
{
public:
    RightValueBehave behave;
    unique_ptr<SymIdNode> id_ptr;
    unique_ptr<Expr> subExpr = nullptr;
    static constexpr std::array<std::u8string_view,3> SupportProd=
    {u8"Expr -> id ", u8"Expr -> id [ Expr ]", u8"Expr -> & id"};
    unique_ptr<SymIdNode> idNode;

};

class ConstExpr : public Expr
{
public:
    SymType Type;
    std::variant<int,float,uint64_t> value;
    static constexpr std::array<std::u8string_view,2> SupportProd=
    {u8"Expr -> num ",u8"Expr -> flo"};
    ConstExpr() {
        this->Ntype = ASTType::Expr;
        this->subType = ASTSubType::ConstExpr;
    }
    ~ConstExpr() {}
};

class DerefExpr : public Expr 
{
public:
    unique_ptr<Expr> subExpr;
    static constexpr std::array<std::u8string_view,1> SupportProd=
    {u8"Expr -> * Expr "};
};

class CallExpr : public Expr
{
public:
    unique_ptr<SymIdNode> id_ptr;
    unique_ptr<ASTNode> ParamList_ptr;
    static constexpr std::array<std::u8string_view,1> SupportProd=
    {u8"Expr -> id ( ParamList ) "};
};

enum BinaryOpEnum {
    ADD,
    MUL,
};

class ArithExpr : public Expr 
{
public:
    unique_ptr<Expr> Lval_ptr;
    BinaryOpEnum Op;
    unique_ptr<Expr> Rval_ptr;
    static constexpr std::array<std::u8string_view,2> SupportProd=
    {u8"Expr -> Expr + Expr ",u8"Expr -> Expr * Expr"};
};



}
#endif