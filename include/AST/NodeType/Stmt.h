#ifndef M_AST_STMT_TYPE_HEADER
#define M_AST_STMT_TYPE_HEADER
#include "AST/NodeType/ASTBool.h"
#include"AST/NodeType/ASTbaseType.h"
#include"AST/NodeType/Expr.h"
#include "AST/NodeType/NodeBase.h"
#include "AST/NodeType/Param.h"
#include "AST/NodeType/Program.h"
#include "parserGen.h"
#include <cassert>
#include <cstddef>
#include <memory>
#include <optional>
namespace AST
{
    
//特别注意，需要支持Block
//class Block;


class BlockStmt : public Stmt 
{
public :
    static constexpr std::array<std::u8string_view,1> SupportProd=
    {u8"Stmt -> Block "};
    unique_ptr<Block> block_ptr;
    inline BlockStmt() {
        this->Ntype = ASTType::Stmt;
        this->subType = ASTSubType::BlockStmt;
    }  
    inline ~BlockStmt(){}
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
            // Block
            assert(NonTnode->childs.size() == 1);
            assert(dynamic_cast<Block *>(NonTnode->childs[0].get()) && "不是Block类型节点");
            
            auto newNode = std::make_unique<BlockStmt>();

            newNode->block_ptr.reset(static_cast<Block*>(NonTnode->childs[0].release()));
            return newNode;
        }
        default:
            std::cerr <<"Not implement BlockStmt Node :"<< targetProd<<std::endl;
            return nullptr;
        }
    }
    unique_ptr<ASTNode> try_construct(ASTNode * as , AbstractSyntaxTree * astTree) override {
        return BlockStmt::try_constructS(as,astTree);
    }
    inline void accept(ASTVisitor & visitor) override {
        visitor.enter(this);

        visitor.visit(this);
        this->block_ptr->accept(visitor);
        
        visitor.quit(this);
        return;
    }
};
class Assign : public Stmt 
{
public :
    static constexpr std::array<std::u8string_view,2> SupportProd=
    {u8"Stmt -> id = Expr ;",u8"Stmt -> Expr = Expr ; "};
    unique_ptr<SymIdNode> id_ptr = nullptr;
    unique_ptr<Expr> Lexpr_ptr = nullptr;
    unique_ptr<Expr> Rexpr_ptr;
    inline Assign() {
        this->Ntype = ASTType::Stmt;
        this->subType = ASTSubType::Assign;
    }
    inline ~Assign() {}
    static unique_ptr<ASTNode> try_constructS(ASTNode * as , AbstractSyntaxTree * astTree);
    unique_ptr<ASTNode> try_construct(ASTNode * as , AbstractSyntaxTree * astTree) override {
        return Assign::try_constructS(as,astTree);
    }
    inline void accept(ASTVisitor & visitor) override {
        visitor.enter(this);

        this->Rexpr_ptr->accept(visitor);
        if(this->id_ptr == nullptr) {
            this->Lexpr_ptr->accept(visitor);
        }
        else {
            this->id_ptr->accept(visitor);
        }        

        visitor.visit(this);
        visitor.quit(this);
        return;
    }
};

enum BranchType {
    BRANCH_NONE_INIT,
    ifOnly,
    ifelse,
    switchcase,
    gototag,
};

class Branch : public Stmt 
{
public:
    BranchType op = BRANCH_NONE_INIT;
    unique_ptr<ASTBool> bool_ptr;
    unique_ptr<Stmt> if_stmt_ptr;
    std::optional<unique_ptr<Stmt>> else_stmt_ptr;
    static constexpr std::array<std::u8string_view,2> SupportProd=
    {u8"Stmt -> if ( Bool ) Stmt",u8"Stmt -> if ( Bool ) Stmt else Stmt "};
    Branch() {
        this->Ntype = ASTType::Stmt;
        this->subType = ASTSubType::Branch;
    }
    ~Branch() {}
    static unique_ptr<ASTNode> try_constructS(ASTNode * as , AbstractSyntaxTree * astTree);
    unique_ptr<ASTNode> try_construct(ASTNode * as , AbstractSyntaxTree * astTree) override {
        return Branch::try_constructS(as,astTree);
    }
    inline void accept(ASTVisitor & visitor) override {
        visitor.enter(this);
        bool_ptr->accept(visitor);
        visitor.visit(this);
        if_stmt_ptr->accept(visitor);
        if(else_stmt_ptr.has_value()) {
            else_stmt_ptr.value()->accept(visitor);
        }
        visitor.quit(this);
        return;
    }

};

enum LoopType {
    NOT_INIT_LOOP,
    whileLOOP,
};

class Loop : public Stmt 
{
public:
    LoopType op = LoopType::NOT_INIT_LOOP;
    unique_ptr<ASTBool> bool_ptr;
    unique_ptr<Stmt> stmt_ptr;
    static constexpr std::array<std::u8string_view,1> SupportProd=
    {u8"Stmt -> while ( Bool ) Stmt "};
    static unique_ptr<ASTNode> try_constructS(ASTNode * as , AbstractSyntaxTree * astTree);
    unique_ptr<ASTNode> try_construct(ASTNode * as , AbstractSyntaxTree * astTree) override {
        return Loop::try_constructS(as,astTree);
    }
    inline void accept(ASTVisitor & visitor) override {
        visitor.enter(this);
        bool_ptr->accept(visitor);
        visitor.visit(this);
        stmt_ptr->accept(visitor);
        visitor.quit(this);
        return;
    }
};

class FunctionCall : public Stmt 
{
public:
    unique_ptr<SymIdNode> id_ptr;
    unique_ptr<ParamList> paramList_ptr;
    static constexpr std::array<std::u8string_view,1> SupportProd=
    {u8"Stmt -> id ( ParamList ) ; "};
    inline FunctionCall() {
        this->Ntype = ASTType::Stmt;
        this->subType = ASTSubType::FunctionCall;
    }
    inline ~FunctionCall() {}
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
            //id ( ParamList ) ;
            assert(NonTnode->childs.size() == 5);
            
            assert(dynamic_cast<ParamList *>(NonTnode->childs[2].get())&&"不是Stmt类型节点");
            auto newNode = std::make_unique<FunctionCall>();
            
            newNode->id_ptr = std::make_unique<SymIdNode>();
            newNode->id_ptr->Literal = static_cast<TermSymNode*>(NonTnode->childs[1].get())->value;
            newNode->paramList_ptr.reset(static_cast<ParamList*>(NonTnode->childs[2].release()));
            
            return newNode;
        }
        default:
            std::cerr <<"Not implement FunctionCall Node :"<< targetProd<<std::endl;
            return nullptr;
        }
    }
    unique_ptr<ASTNode> try_construct(ASTNode * as , AbstractSyntaxTree * astTree) override {
        return Loop::try_constructS(as,astTree);
    }
    inline void accept(ASTVisitor & visitor) override {
        visitor.enter(this);
        this->paramList_ptr->accept(visitor);
        this->id_ptr->accept(visitor);
        visitor.visit(this);
        visitor.quit(this);
        return;
    }
};

class Return : public Stmt 
{
public:
    std::optional<unique_ptr<Expr>> expr_ptr;
    static constexpr std::array<std::u8string_view,2> SupportProd=
    {u8"Stmt -> return Expr ;",u8"return ;"};
    Return() {
        this->Ntype = ASTType::Stmt;
        this->subType = ASTSubType::Return;
    }
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
            //return Expr ;
            assert(NonTnode->childs.size() == 3);
            assert(dynamic_cast<Expr *>(NonTnode->childs[1].get())&&"不是Stmt类型节点");
            auto newNode = std::make_unique<Return>();
            newNode->expr_ptr = nullptr;
            newNode->expr_ptr.value().reset(static_cast<Expr*>(NonTnode->childs[1].release()));
            
            return newNode;
        }
        case 1:
        {
            //return ;
            assert(NonTnode->childs.size() == 3);
            auto newNode = std::make_unique<Return>();
            newNode->expr_ptr = std::nullopt;
            
            return newNode;
        }
        default:
            std::cerr <<"Not implement FunctionCall Node :"<< targetProd<<std::endl;
            return nullptr;
        }
    }
    unique_ptr<ASTNode> try_construct(ASTNode * as , AbstractSyntaxTree * astTree) override {
        return Return::try_constructS(as,astTree);
    }
    inline void accept(ASTVisitor & visitor) override {
        visitor.enter(this);
        if(this->expr_ptr.has_value())
            this->expr_ptr.value()->accept(visitor);
        visitor.visit(this);
        visitor.quit(this);
        return;
    }
};


//Not Implement
class StmtPrint : public Stmt 
{
public:
    static constexpr std::array<std::u8string_view,1> SupportProd=
    {u8"Stmt -> print Expr ;"};
};

} // namespace AST


#endif