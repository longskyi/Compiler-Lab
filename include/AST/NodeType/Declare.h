#ifndef M_AST_DECLARE_TYPE_HEADER
#define M_AST_DECLARE_TYPE_HEADER
#include<optional>
#include"AST/NodeType/ASTbaseType.h"
#include"AST/NodeType/Type.h"
#include"AST/NodeType/Expr.h"
#include"AST/NodeType/Arg.h"
#include"AST/NodeType/Program.h"
namespace AST
{
    

class IdDeclare : public Declare 
{
public:
    SymType id_type;
    unique_ptr<SymIdNode> id_ptr;
    std::optional<int> array_len;
    std::optional<unique_ptr<Expr>> initExpr;
    inline IdDeclare() {
        this->Ntype = ASTType::Declare;
        this->subType = ASTSubType::IdDeclare;
    }
    static constexpr std::array<std::u8string_view,3> SupportProd=
    {u8"Decl -> Type id ;",u8"Decl -> Type id = Expr ; ",u8"Decl -> Type id [ num ] ;" };
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
            //Type id ;
            assert(NonTnode->childs.size() == 3);
            assert(dynamic_cast<pType *>(NonTnode->childs[0].get()) && "是基本类型节点");
            assert(dynamic_cast<TermSymNode*>(NonTnode->childs[1].get()));
            assert(dynamic_cast<TermSymNode*>(NonTnode->childs[1].get())->token_type == u8"ID");

            auto newNode = std::make_unique<IdDeclare>();
            newNode->id_type = std::move((static_cast<pType*>(NonTnode->childs[0].get()))->type);
            newNode->id_ptr = std::make_unique<SymIdNode>();
            newNode->id_ptr->Literal = static_cast<TermSymNode*>(NonTnode->childs[1].get())->value;
            return newNode;
        }
        case 1:
        {
            //Type id = Expr;
            assert(NonTnode->childs.size() == 5);
            assert(dynamic_cast<pType *>(NonTnode->childs[0].get()) && "是基本类型节点");
            assert(dynamic_cast<TermSymNode*>(NonTnode->childs[1].get()));
            assert(dynamic_cast<TermSymNode*>(NonTnode->childs[1].get())->token_type == u8"ID");
            assert(dynamic_cast<Expr *>(NonTnode->childs[3].get()) && "是Expr类型节点");
            auto newNode = std::make_unique<IdDeclare>();
            newNode->id_type = std::move((static_cast<pType*>(NonTnode->childs[0].get()))->type);
            newNode->id_ptr = std::make_unique<SymIdNode>();
            newNode->id_ptr->Literal = static_cast<TermSymNode*>(NonTnode->childs[1].get())->value;
            newNode->initExpr = nullptr;
            newNode->initExpr.value().reset(static_cast<Expr *>(NonTnode->childs[3].release()));
            return newNode;
        }
        case 2:
        {
            //Type id [ num ] ;
            assert(NonTnode->childs.size() == 6);
            assert(dynamic_cast<pType *>(NonTnode->childs[0].get()) && "是基本类型节点");
            assert(dynamic_cast<TermSymNode*>(NonTnode->childs[1].get()));
            assert(dynamic_cast<TermSymNode*>(NonTnode->childs[1].get())->token_type == u8"ID");
            assert(dynamic_cast<TermSymNode*>(NonTnode->childs[3].get())->token_type == u8"NUM" && "是NUM类型节点");
            auto newNode = std::make_unique<IdDeclare>();
            newNode->id_type =std::move((static_cast<pType*>(NonTnode->childs[0].get()))->type);
            newNode->id_ptr = std::make_unique<SymIdNode>();
            newNode->id_ptr->Literal = static_cast<TermSymNode*>(NonTnode->childs[1].get())->value;
            newNode->array_len = std::stoi(toString(static_cast<TermSymNode*>(NonTnode->childs[3].get())->value));
            return newNode;
        }
        default:
            std::cerr <<"Not implement IdDeclare Node :"<< targetProd<<std::endl;
            return nullptr;
        }
    }
    unique_ptr<ASTNode> try_construct(ASTNode * as , AbstractSyntaxTree * astTree) override {
        return IdDeclare::try_constructS(as,astTree);
    }
    inline void accept(ASTVisitor& visitor) override {
        visitor.enter(this);
        if(initExpr.has_value()) {
            initExpr.value()->accept(visitor);
        }
        id_ptr->accept(visitor);
        visitor.visit(this);
        visitor.quit(this);
        return;
    }
};

class FuncDeclare : public Declare 
{
public:
    SymType funcRetType;
    unique_ptr<SymIdNode> id_ptr;
    unique_ptr<ArgList> ArgList_ptr;
    unique_ptr<Block> Block_ptr;
    static constexpr std::array<std::u8string_view,1> SupportProd=
    {u8"Decl -> Type id ( ArgList ) Block"};
    inline FuncDeclare() {
        this->Ntype = ASTType::Declare;
        this->subType = ASTSubType::FuncDeclare;
    }
    static unique_ptr<ASTNode> try_constructS(ASTNode * as , AbstractSyntaxTree * astTree);
    unique_ptr<ASTNode> try_construct(ASTNode * as , AbstractSyntaxTree * astTree) override {
        return FuncDeclare::try_constructS(as,astTree);
    }
    void accept(ASTVisitor& visitor) override;
};



} // namespace AST


#endif