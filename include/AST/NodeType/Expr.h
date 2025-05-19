#ifndef M_AST_EXPRTYPE_HEADER
#define M_AST_EXPRTYPE_HEADER
#include"fileIO.h"
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
    inline static unique_ptr<ASTNode> try_constructS(ASTNode * as , AbstractSyntaxTree * astTree) {
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
                newNode->Type.Type = baseType::INT;
            }
            else if(TermNode->token_type == u8"FLO") {
                newNode->value = std::stof(toString(TermNode->value));
                newNode->Type.Type = baseType::FLOAT;
            }
            else {
                std::unreachable();
            }
            return newNode;
        }
    }
    unique_ptr<ASTNode> try_construct(ASTNode * as , AbstractSyntaxTree * astTree) override {
        return ConstExpr::try_constructS(as,astTree);
    }
    ConstExpr() {
        this->Ntype = ASTType::Expr;
        this->subType = ASTSubType::ConstExpr;
    }
    inline void accept(ASTVisitor& visitor) override {
        visitor.enter(this);
        visitor.visit(this);
        visitor.quit(this);
        return;
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