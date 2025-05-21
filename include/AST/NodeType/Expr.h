#ifndef M_AST_EXPRTYPE_HEADER
#define M_AST_EXPRTYPE_HEADER
#include "SyntaxType.h"
#include "NodeBase.h"
#include "lcmpfileio.h"
#include"AST/NodeType/ASTbaseType.h"
#include"AST/NodeType/Param.h"
#include <cstdlib>
#include <iostream>
namespace AST
{



enum RightValueBehave {
    NO_INIT,
    ref,
    direct,
};

class IdValueExpr : public Expr
{
public:
    RightValueBehave behave;
    unique_ptr<SymIdNode> id_ptr;
    unique_ptr<Expr> subExpr = nullptr;
    static constexpr std::array<std::u8string_view,2> SupportProd=
    {u8"Expr -> id ", u8"Expr -> & id"};
    inline static unique_ptr<ASTNode> try_constructS(ASTNode * as , AbstractSyntaxTree * astTree) {
        auto * NonTnode = dynamic_cast<NonTermProdNode *>(as);
        if(!NonTnode) {
            std::cerr << "Compiler internal Error ptype 不接受非NonTermProdNode的try contrust请求";
            return nullptr;
            //throw std::runtime_error("ERROR,ConstExpr不接受非NonTermProdNode的try contrust请求");
        }
        //检查 str prod匹配
        int targetProd = 0;
        for(targetProd = 0 ; targetProd < SupportProd.size() ; targetProd++) {
            auto && u8prod = LCMPFileIO::parseProduction(SupportProd[targetProd]);
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
            // id
            assert(NonTnode->childs.size() == 1);
            

            auto newNode = std::make_unique<IdValueExpr>();

            assert(dynamic_cast<TermSymNode *>(NonTnode->childs[0].get())->token_type == u8"ID" && "不是id类型节点");
            newNode->id_ptr = std::make_unique<SymIdNode>();
            newNode->id_ptr->Literal = static_cast<TermSymNode*>(NonTnode->childs[0].get())->value;
            newNode->behave = RightValueBehave::direct;

            return newNode;
        }
        case 1:
        {
            // & id 
            assert(NonTnode->childs.size() == 2);
            assert(dynamic_cast<TermSymNode* >(NonTnode->childs[0].get())->token_type == u8"DEREF" && "不是REF");

            auto newNode = std::make_unique<IdValueExpr>();
            
            assert(dynamic_cast<TermSymNode *>(NonTnode->childs[0].get())->token_type == u8"ID" && "不是id类型节点");
            newNode->id_ptr = std::make_unique<SymIdNode>();
            newNode->id_ptr->Literal = static_cast<TermSymNode*>(NonTnode->childs[0].get())->value;

            newNode->subExpr.reset(static_cast<Expr *>(NonTnode->childs[2].release()));
            newNode->behave = RightValueBehave::ref;

            return newNode;
        }
        default:
            std::cerr <<"Not implement ArithExpr Node :"<< targetProd<<std::endl;
            return nullptr;
        }
    }
    unique_ptr<ASTNode> try_construct(ASTNode * as , AbstractSyntaxTree * astTree) override {
        return IdValueExpr::try_constructS(as,astTree);
    }
    inline void accept(ASTVisitor & visitor) override {
        visitor.enter(this);
        this->id_ptr->accept(visitor);
        visitor.visit(this);
        visitor.quit(this);
    }

};

class ConstExpr : public Expr
{
public:
    SymType Type;
    std::variant<int,float,uint64_t> value;
    static constexpr std::array<std::u8string_view,2> SupportProd=
    {u8"Expr -> num ",u8"Expr -> flo"};
    static unique_ptr<ASTNode> try_constructS(ASTNode * as , AbstractSyntaxTree * astTree);
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
    static constexpr std::array<std::u8string_view,2> SupportProd=
    {u8"Expr -> * Expr ",u8"Expr -> Expr [ Expr ]"};
    DerefExpr() {
        this->Ntype = ASTType::Expr;
        this->subType = ASTSubType::DerefExpr;
    }
    static unique_ptr<ASTNode> try_constructS(ASTNode * as , AbstractSyntaxTree * astTree);
    unique_ptr<ASTNode> try_construct(ASTNode * as , AbstractSyntaxTree * astTree) override {
        return DerefExpr::try_constructS(as,astTree);
    }
    inline void accept(ASTVisitor & visitor) override {
        visitor.enter(this);
        this->subExpr->accept(visitor);
        visitor.visit(this);
        visitor.quit(this);
    }
};

class CallExpr : public Expr
{
public:
    unique_ptr<SymIdNode> id_ptr;
    unique_ptr<ParamList> ParamList_ptr;
    static constexpr std::array<std::u8string_view,1> SupportProd=
    {u8"Expr -> id ( ParamList ) "};
    CallExpr() {
        this->Ntype = ASTType::Expr;
        this->subType = ASTSubType::CallExpr;
    }
    static unique_ptr<ASTNode> try_constructS(ASTNode * as , AbstractSyntaxTree * astTree);
    unique_ptr<ASTNode> try_construct(ASTNode * as , AbstractSyntaxTree * astTree) override {
        return CallExpr::try_constructS(as,astTree);
    }
    void accept(ASTVisitor & visitor) override;
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
    ArithExpr() {
        this->Ntype = ASTType::Expr;
        this->subType = ASTSubType::ArithExpr;
    }
    static unique_ptr<ASTNode> try_constructS(ASTNode * as , AbstractSyntaxTree * astTree);
    unique_ptr<ASTNode> try_construct(ASTNode * as , AbstractSyntaxTree * astTree) override {
        return ArithExpr::try_constructS(as,astTree);
    }
    inline void accept(ASTVisitor & visitor) override {
        visitor.enter(this);

        Lval_ptr->accept(visitor);
        Rval_ptr->accept(visitor);
        visitor.visit(this);

        visitor.quit(this);
        return;
    }
};



}
#endif