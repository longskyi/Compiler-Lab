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

class Dimensions : public ASTNode {
public:
    std::vector<int> array_len_vec;
    inline Dimensions() {
        this->Ntype = ASTType::Dimensions;
        this->subType = ASTSubType::Dimensions;
    }
    static constexpr std::array<std::u8string_view,2> SupportProd=
    {u8"Dimensions -> [ num ]",u8"Dimensions -> Dimensions [ num ]" };
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
            //[ num ];
            assert(NonTnode->childs.size() == 3);
            assert(dynamic_cast<TermSymNode*>(NonTnode->childs[1].get()));
            assert(dynamic_cast<TermSymNode*>(NonTnode->childs[1].get())->token_type == u8"NUM");

            auto newNode = std::make_unique<Dimensions>();
            newNode->array_len_vec.push_back(std::stoi(toString(static_cast<TermSymNode*>(NonTnode->childs[1].get())->value)));
            return newNode;
        }
        case 1:
        {
            //Dimensions [ num ]
            assert(NonTnode->childs.size() == 4);
            assert(dynamic_cast<Dimensions *>(NonTnode->childs[0].get()) && "是基本类型节点");
            assert(dynamic_cast<TermSymNode*>(NonTnode->childs[2].get())->token_type == u8"NUM");
            unique_ptr<Dimensions> stoleNode;
            stoleNode.reset(static_cast<Dimensions *>(NonTnode->childs[0].release()));
            stoleNode->array_len_vec.push_back(std::stoi(toString(static_cast<TermSymNode*>(NonTnode->childs[2].get())->value)));
            return stoleNode;
        }
        default:
            std::cerr <<"Not implement IdDeclare Node :"<< targetProd<<std::endl;
            return nullptr;
        }
    }
    unique_ptr<ASTNode> try_construct(ASTNode * as , AbstractSyntaxTree * astTree) override {
        return Dimensions::try_constructS(as,astTree);
    }
    inline void accept(ASTVisitor& visitor) override {
        visitor.enter(this);
        visitor.visit(this);
        visitor.quit(this);
        return;
    }
};

}

#endif